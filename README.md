# mingw-w64 runtime signatures for rizin (GCC 12, 13, 14)

Two FLIRT signature files that give names to the statically linked runtime of a
Windows program built with GCC and mingw-w64: the C runtime startup, libmingwex,
winpthreads, libgcc and libstdc++. They are for anyone who opens a stripped,
statically linked GNU-built PE in rizin and has to find the author's code among
thousands of functions called `fcn.14000a3b0`.

| File | Signatures | Bytes | sha256 |
|---|--:|--:|---|
| `sigs/mingw-w64-gcc12-14_x86_32.sig` | 4,334 | 1,512,979 | `6602727d38a743c83fba21a383a2687801b65809d4526b77a348f193fb239409` |
| `sigs/mingw-w64-gcc12-14_x86_64.sig` | 3,867 | 1,438,074 | `2ffe24a5f476468603835160e8d1a8e209f30135b8716b55591f31f096a70bb9` |

Verify a download from the repository root:

```
cd sigs && sha256sum -c ../recipe/expected.sha256
```

That checks the two `.sig` files and the two `.pat` intermediates they were
converted from, which ship beside them.

Counting them yourself takes one command, and it disagrees with the file's own
header:

```
rz-sign -c /tmp/out.pat sigs/mingw-w64-gcc12-14_x86_32.sig
grep -vc '^---$' /tmp/out.pat          # 4334
```

`rz-sign -d` prints `Built with rizin 0.9.1, 3065 modules` for that same file.
That number counts rizin's internal tree nodes, not signatures, and is always
lower. The counts in this README are pattern lines.

## The gap this fills

rizin 0.9.1 installs a signature database of 76 files, 37 for ELF and 39 for PE.
The PE side is 32 Visual Studio files (2005 through 2022, across arm/32, arm/64,
mips/32, sh/32, x86/32 and x86/64), 4 Windows SDK, 1 MASM32 — and for GNU-linked
PEs exactly two, `pe/x86/32/mingw32-zlib.sig` and `pe/x86/64/mingw32-zlib.sig`,
134 and 141 signatures of zlib 1.3. Apart from `snprintf` and `vsnprintf`, which
came along with those zlib builds, none of it is mingw-w64 runtime code. The
`ubuntu-libgcc` and `ubuntu-libstdc++` files are real, and they are under
`sigdb/elf/`, so they never match a Windows PE.

Check it in half a minute, wherever your rizin keeps its sigdb:

```
find "$(rizin -H RZ_PREFIX)/share/rizin/sigdb" -name '*.sig' | wc -l    # 76
find "$(rizin -H RZ_PREFIX)/share/rizin/sigdb/pe" -name '*.sig' \
  | grep -vi visualstudio                                              # winsdk, masm32, mingw32-zlib
```

So a statically linked mingw-w64 PE opens with its whole runtime unnamed. These
two files name most of it.

I did not check the signature sets that ship with other disassemblers, IDA Pro
among them. This is a statement about rizin's own database and nothing wider.

## Use

Analyze first, then apply. FLIRT matches against the functions rizin has already
found, so a file applied before analysis matches nothing.

```
rizin -q -c 'aa; aac; Fs mingw-w64-gcc12-14_x86_64.sig; afl~flirt.' sample.exe
```

Use them on x86 and x64 PEs that GNU ld linked: `MajorLinkerVersion` 2 in the
optional header, no Visual Studio Rich header, and not packed. The cheap tell:

```
rizin -q -c 'i~linker' sample.exe
```

`Linker 02.41` or another `02.x` is GNU ld. An MSVC binary reports `14.x`. On a
binary that is not GNU-linked these signatures cannot contribute a right name,
only a wrong one.

**Do not copy them into rizin's sigdb directory.** `aaa` and `Fa` apply
everything they find there to every PE of the matching architecture, which would
put this pack against MSVC-linked binaries as well. Keep it outside and name it
on the command line.

## Check it works

Build one of the training programs, strip it, and compare. With the pinned GCC
13 toolchain on x64, measured today:

```
x86_64-w64-mingw32-gcc -O2 -municode recipe/src/tr_wmain.c -static -o tr.exe
x86_64-w64-mingw32-strip tr.exe
rizin -q -c 'aa; aac; afl~?' tr.exe                                            # 94
rizin -q -c 'aa; aac; Fs sigs/mingw-w64-gcc12-14_x86_64.sig; afl~flirt?' tr.exe # 47
```

All 47 are runtime code: the mingwex printf engine (`__mingw_vfprintf`,
`__pformat_*`), David Gay's dtoa (`*_D2A`), the CRT startup
(`__tmainCRTStartup`) and the pseudo-relocation runtime
(`_pei386_runtime_relocator`). The program's own `wmain` keeps no name, which is
the point of the design below.

## What it covers

GCC 12.2 with mingw-w64 10 (Debian 12), GCC 13.2 with mingw-w64 11 (Ubuntu
24.04, both win32 and posix threads) and GCC 14.2 with mingw-w64 12 (Debian 13),
for 32- and 64-bit PEs. The signed archives are `crt2.o` and `dllcrt2.o` and
their variants, `libmingw32`, `libmingwex`, the static members of `libmsvcrt`,
`libwinpthread`, `libgcc`, `libgcc_eh` and `libstdc++`.

GCC's `libssp` was linked into the training programs and no `libssp` function is
in the pack: every one of them is shorter than the 48-byte minimum below.

## What it does not cover

Not in the pack, and not measured, so I have no idea how they behave:

- MSYS2 and UCRT toolchains (GCC 15 with the ucrt runtime);
- llvm-mingw, which is clang with libc++;
- the Rust standard library from the `x86_64-pc-windows-gnu` target;
- SJLJ i686 toolchains;
- the WinLibs, TDM and MinGW-builds distributions.

A Rust or clang binary is not a smaller version of this problem. It is a
different runtime, and these files will name very little of it.

## Measured

Library functions named in a stripped, statically linked mingw PE:

| signatures applied | x86 C | x86-64 C | x86-64 C++ |
|---|---|---|---|
| **this pack (GCC 12 + 13 + 14)** | **82.3%** | **80.9%** | **80.6%** |
| GCC 13 only | 37.2% | 80.9% | 25.0% |
| none (rizin's own analysis) | 20.0% | — | 20.7% |

Cost: **+11.9%** analysis CPU on the GCC 13 set.

Every number here, the method behind it, and the 15.0-20.2% of names that
belong to an identical-code sibling rather than the function itself, are in
[MEASUREMENTS.md](MEASUREMENTS.md). The harness that produced them is in the
repository; nothing above is an estimate.

## Why a separate generator: `Fc` deletes functions

rizin can write signatures itself with `Fc`, and the first version of this pack
was made that way. It deleted `main`.

`Fc` records `rz_analysis_function_size_from_entry()` as the function's length.
That is the distance from the entry to the end of the furthest basic block, so
when the analysis reaches a block placed far from the entry — a cold path after
other functions, or a walk past a call into a noreturn function — the stored
length covers every function in between. On a match,
`module_match_buffer()` grows the matched function to that length whenever its
own analyzed size is smaller, moves the blocks of every function that starts
inside the grown range into it, and deletes those functions with
`rz_analysis_function_delete()`. Nothing compares the stored length against the
functions already there.

Measured once while building the pack, in September 2026: `Fc` on 16 unstripped
GCC 13 mingw builds, x86 and x64, wrote 242 signatures whose stored length
exceeded the function's true size (from `.pdata`, or the next COFF symbol), by
up to 1,068,822 bytes — `__Unwind_GetGR` is 55 bytes and was stored as
1,068,877. Applied to the 24 stripped held-out files, those signatures deleted
157 of the programs' own functions, `main` among them, and 2,001 runtime
functions. The working directory of that run no longer exists, so `eval/` does
not reproduce those four counts. What `eval/` does reproduce is that this pack
deletes nothing: author functions deleted by a match, 0.

The behavior itself reproduces from one 52-line assembly file in under a minute,
and `UPSTREAM-ISSUE-DRAFT.md` has it: the same 307-byte function in two layouts,
`Fc` storing 339 bytes for it, and `main` and `helper` gone from `afl` after the
match. That draft is an unfiled issue for rizin, with two alternative fixes.
I have not filed it yet.

So `recipe/lib/patgen.py` writes the patterns instead. It signs only functions
the linker map places in a toolchain object, at their true size — `.pdata` on
x64; on x86 the next symbol in the same input section, less padding — and masks
only what changes between builds: RIP-relative displacements, branch targets
leaving the function, and `.reloc` entries.

Two more guards come out of the same kind of measurement.

**A 48-byte minimum.** Short functions compile to the same bytes whoever wrote
them: getters, constant returns, varargs wrappers. GCC 13's signatures alone,
over the 72 test files:

| Minimum length | Author-written functions matched | of which renamed | Library template code in author objects |
|---|--:|--:|--:|
| none | 140 | 134 | 30 |
| 32 bytes | 7 | 7 | 28 |
| 40 bytes | 1 | 1 | 0 |
| 48 bytes | 1 | 1 | 0 |
| 48 bytes, tail bytes dropped | 8 | 8 | 2 |

The 6 matched but not renamed with no minimum are `ho_ready`, exported by the
six builds of the test DLL; rizin keeps an exported function's own name and
marks the match with a `flirt.*` flag only. The last author functions to go, at
33 to 35 bytes, were varargs wrappers: Lua's `luaO_pushfstring` in four builds,
and SQLite's `sqlite3MPrintf` and `sqlite3_str_appendf`, which compile to the
same bytes as mingw-w64's inline `sprintf`. One Lua build matched
`__mingw_fscanf` instead. 48 bytes leaves 13 bytes of margin above them. The
minimum is in the shipped data as well as in the recipe: the shortest stored
length in either file is exactly 48.

**Tail bytes kept.** `rz-sign` drops the tail bytes by default
(`flirt.node.optimize=2`); the recipe converts with `-e flirt.node.optimize=1`.
Without them `throw E5(m)` in author code matches `std::__throw_logic_error`,
and the 48-byte row above becomes 8 author functions named instead of 1.

## Known limits, and how this was built

Read this before you trust an output. Everything in it is measured, not
estimated, and it is here rather than at the bottom because it is the part that
decides whether the pack is useful to you.

**What it gets wrong, by construction.** 15.0% to 20.2% of the names a match
produces belong to an identical-code sibling rather than the function itself:
`char` and `wchar_t` twins, C1 and C2 constructors, `sin` and `cos`. The bytes
really are identical, so no signature format can separate them. Treat a name as
"this is one of a known family" rather than as proof of the exact function.
`sigs/*.manifest.json` lists, per signature, every name that shares it, so you
can see which ones are ambiguous before you rely on one.

**Toolchains it does not know.** MSYS2/UCRT (GCC 15, ucrt), llvm-mingw (clang,
libc++) and Rust are not covered and were not measured. A binary from one of
those gets a fraction of the coverage above — on plain C programs built by a
GCC version outside 12-14, 2.5% to 12.3%.

**Where it must not be used.** Never add these to rizin's system sigdb. They are
for PEs linked by GNU ld (MajorLinkerVersion 2, no VS Rich header, not
UPX-packed) and applying them to an MSVC binary will name functions wrongly.

**How it was built, since the question is fair.** The signatures come from
executables I compiled and linked myself from the four programs in `recipe/src`,
with the toolchain versions pinned in `recipe/debs.lock`. `recipe/lib/patgen.py`
is my generator and `recipe.sh` rebuilds both files byte for byte against
`recipe/expected.sha256` — six files, all verifiable with one command.

I wrote this with LLM assistance, as I write most things. That is not a
disclaimer, because it does not change what you should check: every number in
this repository comes from the harness, every claim I could not reproduce has
been removed, and the one approach that looked obvious and turned out to destroy
data is documented above rather than quietly avoided. Check the hashes, read
`patgen.py`, re-run the harness. Those are the same things you should do with
code written any other way.

**If you find it wrong**, open an issue with the binary (or how to build it) and
the function it misnamed. A mis-identification is a bug in the pack, not in your
reading of it.

## License

The signature and pattern files are **GPL-3.0-or-later** — the same license as
the library code the patterns were taken from. The generator, the four training
programs and the build recipe are **MIT**.

Three quarters of the signatures describe `libstdc++` and `libgcc`. There is a
reading of the GCC Runtime Library Exception on which the data could carry a
permissive license, and I am deliberately not relying on it: an open question
should be answered in the direction that grants nothing I might not own. A
license can be loosened later and never tightened.

This costs you nothing. rizin reads the files as data, so the license does not
reach anything you analyse with them.

Full reasoning, the per-library breakdown and the upstream notices:
[PROVENANCE.md](PROVENANCE.md) and `third-party/`.

## Not affiliated

Not affiliated with or endorsed by the mingw-w64 project, the GNU Project, the
Rizin project or Hex-Rays. "FLIRT" and "IDA" are Hex-Rays trademarks, used here
to name a file format. No Hex-Rays tool was used to build or check this pack.
