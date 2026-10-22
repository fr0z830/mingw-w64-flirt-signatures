#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Angelos Katis (RADECTI)
# SPDX-License-Identifier: MIT
"""mingw-w64 runtime FLIRT signatures for rizin, from binaries WE compiled.

Two subcommands:

  records OUT.jsonl NAME:BIN:MAP:OBJDIR [...]      (needs capstone + pefile)
      For every function symbol of an UNSTRIPPED training binary, attribute it to
      an input section of the linker map. Only functions from toolchain objects
      and archive members (crt*.o, libgcc, libgcc_eh, libstdc++, libmingw32,
      libmingwex, libssp, libwinpthread, the static members of libmsvcrt) are
      kept. The author's own objects (OBJDIR/) and import-library jump thunks
      (jmp [__imp_X]) never produce a signature. Each kept function becomes one
      record: its TRUE extent (.pdata on x64, else the next symbol capped at the
      input section, minus alignment padding) with exactly the layout-dependent
      bytes masked: x64 RIP-relative displacements, rel8/rel32 branch targets
      outside the function, and every .reloc entry.

  pack --bits 32|64 --out X.pat [--min-ext N] RECORDS.jsonl [...]
      Merge records into one IDA-style .pat (rizin reads it, and rz-sign turns it
      into a .sig). Identical signatures from several binaries collapse into one;
      when two DIFFERENT functions share a signature, the alphabetically first
      name set wins (rizin's own behaviour), and --drop-collisions removes them.

recipe.sh drives both steps; README.md has the measurements behind each choice.
The merge keeps the FIRST record seen for a (signature, name set) pair, so the
order of the RECORDS files and of the binaries inside them is part of the
output (19 pairs differ in length only). recipe.sh fixes that order.
"""
import argparse
import bisect
import collections
import json
import re
import struct
import sys

PRE = 32  # RZ_FLIRT_MAX_PRELUDE_SIZE


# ── PE ground truth: COFF function symbols (+ .pdata extents on x64) ──────────
# Adapted from the 2026-09-24 boundary study's gt.py.
def pe_symbols(path):
    d = open(path, "rb").read()
    lfa = struct.unpack_from("<I", d, 0x3C)[0]
    coff = lfa + 4
    _m, nsec, _ts, symptr, nsym, optsz, _ch = struct.unpack_from("<HHIIIHH", d, coff)
    opt = coff + 20
    magic = struct.unpack_from("<H", d, opt)[0]
    if magic == 0x20B:
        base, dd, bits = struct.unpack_from("<Q", d, opt + 24)[0], opt + 112, 64
    else:
        base, dd, bits = struct.unpack_from("<I", d, opt + 28)[0], opt + 96, 32
    exc_rva, exc_size = struct.unpack_from("<II", d, dd + 3 * 8)
    strtab = symptr + nsym * 18
    secs = []
    for i in range(nsec):
        o = opt + optsz + 40 * i
        vsize, va, rawsz, rawptr = struct.unpack_from("<IIII", d, o + 8)
        ch = struct.unpack_from("<I", d, o + 36)[0]
        secs.append({"va": base + va, "vsize": vsize or rawsz, "rva": va, "rawptr": rawptr, "rawsz": rawsz,
                     "exec": bool(ch & 0x20000000) or bool(ch & 0x20)})

    def rva2off(rva):
        for s in secs:
            if s["rva"] <= rva < s["rva"] + max(s["vsize"], s["rawsz"]):
                return s["rawptr"] + (rva - s["rva"])
        return None

    funcs, labels = {}, set()
    i = 0
    while i < nsym:
        o = symptr + 18 * i
        raw = d[o:o + 8]
        if raw[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", raw, 4)[0]
            name = d[strtab + off:d.index(b"\0", strtab + off)].decode("latin1")
        else:
            name = raw.rstrip(b"\0").decode("latin1")
        value, secnum, typ, scl, naux = struct.unpack_from("<IhHBB", d, o + 8)
        i += 1 + naux
        if secnum <= 0 or secnum > len(secs) or not secs[secnum - 1]["exec"]:
            continue
        addr = secs[secnum - 1]["va"] + value
        if scl in (2, 3, 6) and not name.startswith("."):
            labels.add(addr)
        if typ == 0x20 and scl in (2, 3):
            funcs.setdefault(addr, []).append(name)
    pdata = {}
    if exc_rva and exc_size and bits == 64:
        off = rva2off(exc_rva)
        for k in range(exc_size // 12):
            b0, e0, _u = struct.unpack_from("<III", d, off + 12 * k)
            if b0 and e0 > b0:
                pdata[base + b0] = base + e0
    starts = sorted(labels | set(funcs))
    exec_ranges = [(s["va"], s["va"] + s["vsize"]) for s in secs if s["exec"]]
    out = []
    for a, names in sorted(funcs.items()):
        j = bisect.bisect_right(starts, a)
        nxt = starts[j] if j < len(starts) else None
        end = next((hi for lo, hi in exec_ranges if lo <= a < hi), None)
        if nxt is None or (end is not None and nxt > end):
            nxt = end
        out.append({"addr": a, "names": sorted(set(names)),
                    "size": (pdata[a] - a) if a in pdata else None,
                    "bound": (nxt - a) if nxt else None})
    return bits, base, out


# ── linker map: which input section (object / archive member) holds each address
_MAP_IN = re.compile(r"^ (\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S.*)$")
_MAP_NAME = re.compile(r"^ (\S+)\s*$")
_MAP_CONT = re.compile(r"^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S.*)$")
_MAP_OUT = re.compile(r"^(\.\S+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)")


def map_text_sections(path):
    out, cur, pending, started = [], None, None, False
    for line in open(path, encoding="latin1"):
        line = line.rstrip("\n")
        if not started:
            started = line.startswith("Linker script and memory map")
            continue
        m = _MAP_OUT.match(line)
        if m:
            cur, pending = m.group(1), None
            continue
        if cur != ".text":
            continue
        if pending:
            m = _MAP_CONT.match(line)
            pending = None
            if m:
                lo, sz = int(m.group(1), 16), int(m.group(2), 16)
                if sz:
                    out.append((lo, lo + sz, m.group(3).strip()))
                continue
        m = _MAP_IN.match(line)
        if m and not m.group(1).startswith("*"):
            lo, sz = int(m.group(2), 16), int(m.group(3), 16)
            if sz:
                out.append((lo, lo + sz, m.group(4).strip()))
            continue
        m = _MAP_NAME.match(line)
        if m and not m.group(1).startswith("*"):
            pending = m.group(1)
    out.sort()
    return out


# ── masks ─────────────────────────────────────────────────────────────────────
def crc16(data):
    """The CRC-16 the FLIRT format specifies: reflected, polynomial 0x8408,
    initial value 0xFFFF, final complement and byte swap.

    Implemented from the format description, not ported from anyone's source.
    Output matches rizin's flirt_crc16, which is the point — a pattern file
    whose CRC disagrees with the matcher's is silently useless. Named after
    what it computes rather than after the first implementation of it, because
    the previous docstring cited FLAIR and rizin by file and read as though
    this function were taken from them.
    """
    if not data:
        return 0
    crc = 0xFFFF
    for b in data:
        d = b
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if (crc ^ d) & 1 else crc >> 1
            d >>= 1
    crc = ~crc & 0xFFFFFFFF
    return ((crc << 8) | ((crc >> 8) & 0xFF)) & 0xFFFF


def _is_pad(ins, X):
    b = bytes(ins.bytes)
    if ins.mnemonic in ("nop", "int3", "fnop") or (b and not any(b)):
        return True
    ops = ins.operands
    if ins.mnemonic == "lea" and len(ops) == 2 and ops[0].type == X.X86_OP_REG and ops[1].type == X.X86_OP_MEM:
        m = ops[1].mem
        if m.base == ops[0].reg and m.disp == 0 and m.index in (0, X.X86_REG_EIZ, X.X86_REG_RIZ):
            return True
    return (ins.mnemonic in ("mov", "xchg") and len(ops) == 2 and ops[0].type == X.X86_OP_REG
            and ops[1].type == X.X86_OP_REG and ops[0].reg == ops[1].reg)


def trim_padding(md, X, code, addr):
    n = len(code)
    insns = list(md.disasm(code, addr))
    if not insns:
        return n
    covered = insns[-1].address + insns[-1].size - addr
    k = len(insns)
    while k > 1 and _is_pad(insns[k - 1], X):
        k -= 1
    if covered < n and k == len(insns):
        return n
    return insns[k - 1].address + insns[k - 1].size - addr


def make_mask(md, capstone, X, code, addr, relocs):
    n = len(code)
    mask = [True] * n
    pos = 0
    branch = {capstone.CS_GRP_JUMP, capstone.CS_GRP_CALL}
    for ins in md.disasm(code, addr):
        off = ins.address - addr
        if off != pos:
            break
        # RIP-relative displacement: ALWAYS 4 bytes. capstone 5.0.3 reports
        # disp_size=2 when a 0x66 operand-size prefix is present (measured).
        if ins.disp_size and any(o.type == X.X86_OP_MEM and o.mem.base == X.X86_REG_RIP for o in ins.operands):
            for k in range(4):
                if off + ins.disp_offset + k < n:
                    mask[off + ins.disp_offset + k] = False
        if ins.imm_size and set(ins.groups) & branch and capstone.CS_GRP_BRANCH_RELATIVE in ins.groups:
            tgt = ins.operands[0].imm if ins.operands and ins.operands[0].type == X.X86_OP_IMM else None
            if tgt is None or not (addr <= tgt < addr + n):
                for k in range(ins.imm_size):
                    mask[off + ins.imm_offset + k] = False
        pos = off + ins.size
        if pos >= n:
            break
    for k in range(pos, n):
        mask[k] = False
    for k in range(n):
        if addr + k in relocs:
            mask[k] = False
    return mask


def cmd_records(a):
    import capstone
    import pefile
    from capstone import x86 as X
    with open(a.out, "w") as out:
        for spec in a.binaries:
            name, path, mapfile, objdir = spec.split(":")
            bits, base, fns = pe_symbols(path)
            secs = map_text_sections(mapfile)
            los = [s[0] for s in secs]
            pe = pefile.PE(path)
            relocs = set()
            for blk in getattr(pe, "DIRECTORY_ENTRY_BASERELOC", []):
                for e in blk.entries:
                    if e.type in (3, 10):
                        relocs.update(base + e.rva + k for k in range(4 if e.type == 3 else 8))
            md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64 if bits == 64 else capstone.CS_MODE_32)
            md.detail = True
            kept = collections.Counter()
            for f in fns:
                a0 = f["addr"]
                j = bisect.bisect_right(los, a0) - 1
                if j < 0 or not (secs[j][0] <= a0 < secs[j][1]):
                    kept["unattributed"] += 1
                    continue
                obj, sec_end = secs[j][2], secs[j][1]
                if obj.startswith(objdir.rstrip("/") + "/"):
                    kept["author"] += 1
                    continue
                ext = f["size"] or 0
                if not ext:
                    b = f["bound"] or 0
                    b = min(b, sec_end - a0) if b else sec_end - a0
                    code = pe.get_data(a0 - base, b) if b > 0 else b""
                    ext = trim_padding(md, X, code, a0) if code else 0
                if not ext:
                    continue
                code = pe.get_data(a0 - base, ext)
                if len(code) != ext:
                    continue
                if code[:2] == b"\xff\x25" and ext <= 8:
                    kept["import_thunk"] += 1
                    continue
                mask = make_mask(md, capstone, X, code, a0, relocs)
                pre = "".join(f"{code[i]:02X}" if mask[i] else ".." for i in range(min(PRE, ext)))
                pre += ".." * (PRE - min(PRE, ext))
                L = 0
                while PRE + L < ext and L < 255 and mask[PRE + L]:
                    L += 1
                t0 = PRE + L
                tail = "".join(f"{code[i]:02X}" if mask[i] else ".." for i in range(t0, min(ext, t0 + 255))).rstrip(".")
                lib = re.search(r"([^/]+?)(?:\(([^)]+)\))?$", obj)
                out.write(json.dumps({
                    "bin": name, "bits": bits, "addr": a0, "names": f["names"], "lib": lib.group(1) if lib else obj,
                    "ext": ext, "pre": pre, "crc_len": L, "crc16": crc16(code[PRE:PRE + L]), "tail": tail,
                    "pre_def": sum(mask[:min(PRE, ext)]),
                    "tail_def": sum(mask[t0:min(ext, t0 + 255)])}) + "\n")
                kept["runtime"] += 1
            print(f"{name:26s} bits={bits} " + " ".join(f"{k}={v}" for k, v in sorted(kept.items())), flush=True)


def best_name(names):
    return sorted(names, key=lambda n: ("." in n, len(n), n))[0]


def cmd_pack(a):
    groups = collections.OrderedDict()
    stats = collections.Counter()
    for rf in a.records:
        for line in open(rf):
            r = json.loads(line)
            if r["bits"] != a.bits:
                continue
            stats["records"] += 1
            if r["ext"] < a.min_ext:
                stats["dropped_short"] += 1
                continue
            key = (r["pre"], r["crc_len"], r["crc16"], r["tail"])
            g = groups.setdefault(key, collections.OrderedDict())
            g.setdefault(tuple(sorted(r["names"])), r)
    lines, manifest = [], []
    for key, idents in groups.items():
        ids = sorted(idents)
        if len(ids) > 1:
            stats["shared_signatures"] += 1
            if a.drop_collisions:
                continue
        r = idents[ids[0]]
        pre, cl, crc, tail = key
        name = best_name(r["names"])
        lines.append(f"{pre} {cl:02X} {crc:04X} {r['ext']:04X} :0000 {name}" + (f" {tail}" if tail else ""))
        manifest.append({"name": name, "idents": [list(i) for i in ids], "ext": r["ext"], "lib": r.get("lib")})
    lines.sort()
    with open(a.out, "w") as fh:
        fh.write("\n".join(lines) + "\n---\n")
    with open(re.sub(r"\.pat$", ".manifest.json", a.out), "w") as fh:
        json.dump(manifest, fh)
    stats["signatures"] = len(lines)
    print(a.out, dict(stats))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("records")
    r.add_argument("out")
    r.add_argument("binaries", nargs="+", help="NAME:UNSTRIPPED_BIN:LINKER_MAP:AUTHOR_OBJDIR")
    p = sub.add_parser("pack")
    p.add_argument("--bits", type=int, required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--min-ext", type=int, default=48)
    p.add_argument("--drop-collisions", action="store_true")
    p.add_argument("records", nargs="+")
    a = ap.parse_args()
    {"records": cmd_records, "pack": cmd_pack}[a.cmd](a)


if __name__ == "__main__":
    sys.exit(main())
