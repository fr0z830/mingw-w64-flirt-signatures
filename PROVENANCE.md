# Provenance, license and what is actually in the files

> Split out of the README so the README fits a screen. Nothing was cut.

## License and provenance

There are two different kinds of thing in this repository and they are licensed
differently.

| Files | License |
|---|---|
| `recipe/recipe.sh`, `recipe/lib/patgen.py`, `recipe/src/*`, this README | MIT — `LICENSES/MIT.txt` |
| `sigs/*.sig`, `sigs/*.pat` | GPL-3.0-or-later — `LICENSES/GPL-3.0-or-later.txt`, and `LICENSE` at the root |
| `recipe/debs.lock`, `recipe/expected.sha256` | MIT, with the code — they are lists of package versions and hashes |

I wrote the generator and the test programs, so those are mine to give away. The
signature files are derived from compiled runtime code that other people wrote,
so I claim no rights in them beyond the selection and arrangement, and I give
that up as well.

### What is actually inside a signature file

One entry per library function: up to 32 leading bytes of its compiled form, a
16-bit checksum over the bytes that follow, the function's total length, its
name, and up to 255 further literal bytes from the tail. Bytes that depend on
where the linker placed things — RIP-relative displacements, branch targets
leaving the function, relocations — are wildcards.

Measured on the two published files, re-serialized to text with rizin itself
(`rz-sign -c out.pat in.sig`):

| | x86/32 | x86/64 |
|---|---:|---:|
| signatures | 4,334 | 3,867 |
| literal bytes of library code, total | 624,841 | 594,781 |
| per signature, mean / max | 144 / 287 | 154 / 287 |
| share of the function's bytes kept, mean | 56.0% | 54.2% |
| signatures keeping 80% or more | 642 | 536 |
| signatures keeping 95% or more | 1 | 10 |

For a 13,000-byte function the file keeps about 2% of it. For a function near the
48-byte floor it can keep nearly all of it: 1,178 of the 8,201 signatures keep
80% or more of their function's compiled bytes. I would rather write that down
than call the whole thing fragments.

What is not in the files: no source code, no object files, no import libraries,
nothing that can be linked or executed, and no way back to the middle of any
function — that region is a 16-bit checksum.

### Where the signed bytes come from

Every signed byte comes from an archive member named in the linker map of a
binary built by `recipe.sh` from the packages pinned in `debs.lock`:

| Component | Upstream license |
|---|---|
| `crt2`, `dllcrt2`, `libmingw32`, `libmingwex`, static members of `libmsvcrt` | mingw-w64 project, ZPL-2.1 overall, with per-file BSD-2-clause (NetBSD), BSD-3-clause, ISC, Lucent (`gdtoa`), Sun (libm), Keith Marshall, Cygwin and public-domain parts |
| `libwinpthread` | Expat, with BSD-3-clause headers (Lockless Inc.) and public-domain parts |
| `libgcc`, `libgcc_eh`, `libstdc++` | GPL-3.0-or-later WITH GCC-exception-3.1 |
| `libssp` | GPL-3.0-or-later WITH GCC-exception-3.1 — and no libssp function is in the pack, every one of them is under the 48-byte floor |

The `sigs/*.manifest.json` files name, for each signature, the archive member it
came from and every library name that shares its bytes, so the attribution above
is checkable per signature rather than in aggregate.

The two upstream notices that collect the per-file terms are reproduced verbatim
in `third-party/`, taken from the `mingw-w64-common` and `gcc-mingw-w64-base`
packages that `debs.lock` pins. The signatures are transformed extracts of code
from those packages — masked and truncated as described above — produced on
2026-09-26.

Nothing here comes from Microsoft. No signature is derived from `msvcrt.dll`, an
MSVC runtime or the Windows SDK; the build only unpacks the Debian and Ubuntu
packages in `debs.lock`, and import thunks are dropped rather than signed.

### Why I think this is mine to publish

Two reasons that do not depend on each other.

**The data files take the license of the code they describe, and that is a
deliberate choice rather than a legal conclusion.**

76% of the signatures in this pack are of `libstdc++`, with most of the rest
from `libgcc` and the mingw-w64 runtime. Those are GPL-3.0-or-later with the GCC
Runtime Library Exception 3.1, and section 1 of that exception permits conveying
target code built by combining the runtime with independent modules "under terms
of your choice". My independent modules are `recipe/src/*`, which are MIT, so
there is a reading on which these files could carry a permissive license.

I am not relying on that reading. Whether an exception written about *target
code* reaches *data extracted from* target code is an open question, and the
honest way to handle an open question is to pick the answer that grants nothing
I might not own. So the two `.sig` files and the two `.pat` files are
GPL-3.0-or-later: the same license as the library code the patterns were taken
from.

This costs a user nothing. rizin reads the files as data, so the license does
not reach anything you analyse with them, and rizin's own signature database is
LGPL-3.0 while containing signatures of the same GCC runtime. It can also be
loosened later and never tightened, which is the only direction that matters: if
counsel says the exception does reach extracted data, relicensing the data files
more permissively takes one commit, while a permissive grant made today could
not be withdrawn from anyone who had already taken it.

The mingw-w64 parts, and the BSD, ISC, Lucent, Sun and public-domain parts inside
them, permit redistribution in binary form as long as the notices travel with
it, which is what `third-party/` is for.

What the file actually contains is identification data: a table saying that a
function whose bytes look like this is called that, with layout-dependent bytes
wildcarded and the middle of each function replaced by a checksum. Function names
are facts about the libraries, not my work or anyone's expression. That argument
supports a permissive license and I am still not taking one, for the reason
above.

I am not a lawyer. If you hold rights in one of the components above and read
this differently, write to the address in this repository's profile and I will withdraw the
data files and publish the recipe alone — `recipe.sh` rebuilds both `.sig`
files byte for byte against `expected.sha256`, so nothing is lost if the built
files cannot be hosted.

### No endorsement, and whose trademarks these are

This is not affiliated with or endorsed by the mingw-w64 project, the GNU
Project, the Rizin project or Hex-Rays. "FLIRT" and "IDA" are Hex-Rays
trademarks, used here to name a file format. No Hex-Rays tool was used to build
or check this pack: the patterns come from `recipe/lib/patgen.py` and `rz-sign`
converts them.

### No warranty

The MIT disclaimer applies to the code, and the data files come with no warranty
of any kind either. Concretely, from the measurements above: 15.0% to 20.2% of
the names a match produces belong to an identical-code sibling rather than the
function itself (char and wchar_t twins, C1 and C2 constructors, sin and cos),
and one author-written function in 15,030 was given a library name. Check a name
before you build a conclusion on it.