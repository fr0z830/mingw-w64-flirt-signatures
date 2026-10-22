# Measurements, and how to reproduce them

> Split out of the README so the README fits a screen. Nothing was cut.

## Measured

rizin 0.9.1 on stripped files that were never used to build the pack. The truth
for each one is its unstripped twin: the symbol table gives every function, and
the linker map says whether it came from the author's object or the toolchain's.
`eval/` reproduces all of this, and `eval/RESULTS.txt` is the run these numbers
come from.

The test files are 46 builds of my own C and C++ programs (a statistics
calculator, a C++ program with classes, exceptions and threads, a small DLL,
three programs from an earlier function-boundary study) and of SQLite 3.46.1, at
several optimization levels, x86 and x64 — 10 with GCC 12, 24 with GCC 13, 12
with GCC 14; 6 builds of Lua 5.4.7 at -O0, -O2 and -Os, on which the minimum
length was tuned; and 20 builds of C and C++ written to look like runtime code,
tiny getters, varargs wrappers and exception classes.

The figures come from the pass `tn <67 noreturn names>; aa; aac; Fs <pack>;
aanr`, which tells rizin about 67 runtime functions that never return (`exit`,
`abort`, `__cxa_throw`, the `std::__throw_*` helpers; `eval/measure.py` lists
them) and propagates that afterwards. With the plain `aa; aac; Fs` of the Use
section, the three recall figures read 82.3%, 80.8% and 80.6%: rizin finds one
more GCC 13 runtime function without the hints, 6,531 instead of 6,530, and the
pack names the same 5,280.

**Runtime functions named, of those rizin's pass found.**

| | GCC 12 (10 files) | GCC 13 (24) | GCC 14 (12) |
|---|--:|--:|--:|
| this pack | 82.3% (2,477 of 3,010) | 80.9% (5,280 of 6,530) | 80.6% (2,585 of 3,209) |
| GCC 13 signatures only | 37.2% (1,121 of 3,010) | 80.9% (5,280 of 6,530) | 25.0% (803 of 3,209) |

The columns are sets of test files, not versions of the pack. The middle column
is 80.9% on both rows because on GCC 13 files the full pack and a GCC 13-only
pack name the same 5,280 functions; the other two columns are what happens when
the binary's compiler is not the one the signatures came from. On the GCC 12 and
GCC 14 builds of the plain C programs, GCC 13's signatures alone name 2.5% to
12.3% of the runtime functions found, depending on the file. Covering three
compiler versions is most of what this pack is.

**The denominator matters.** These signatures name only functions rizin's
analysis has already found, and in a stripped static mingw PE that is 20.0% to
20.7% of the runtime functions the linker put in the file (3,010 of 15,070;
6,530 of 31,577; 3,209 of 15,594). Most of the rest is library code that nothing
in the program references, which no signature pack can reach. Of the ones rizin
does find, this pack names 80.6% to 82.3%. Those two numbers multiply: do not
read 81% as 81% of the runtime.

**Wrong names.** 15.0% to 20.2% of the names a match produces belong to another
function with the same compiled bytes, usually identical code: a `char` and
`wchar_t` twin, a C1 and C2 constructor pair, `sin` for `cos`. By test set:
20.2% (500 of 2,477), 15.0% (792 of 5,280), 16.0% (414 of 2,585). The
`.manifest.json` files list every name that shares a signature, so you can see
which ones are ambiguous before you trust one.

**Author code.** Over all 72 files, whose authors' objects hold 15,030
functions, the pack gave a library name to one function the author wrote:

```
adv_cpp_O3_64   0x1400015a0   179 bytes
  adv::cat(std::string const&, std::string const&)  ->  flirt.ZStplIcSt11char_traits...
```

It is `std::string cat(const std::string &a, const std::string &b) { return a +
b; }`, which at -O3 on x64 compiles to the same 179 bytes as libstdc++'s
`std::operator+` and gets that name. No byte signature can separate the two.
Seven more hits are library template code the compiler emitted into the author's
own object — `std::string::_M_construct` instantiations and one `std::deque`
member — named after their identical library copies. No author function was
deleted by a match. The first version of this pack did delete them, which is the
next section.

**Cost, on my machine.** Applying the pack added 3.5 s of CPU to the 29.6 s
rizin spent on the 24 GCC 13 files, +11.9%, best of three runs each, and raised
peak memory on the largest of them (SQLite, x86) from 114 to 142 MB. Both depend
on the machine.


## Rebuild it

```
docker build -t rizin-flirt:0.9.1 - < Dockerfile
OUT=/some/empty/dir recipe/recipe.sh
```

### What you need

**An x86-64 machine running Ubuntu 24.04.** GCC 13 comes from the host rather
than from a pinned package, so the recipe checks these eleven packages and stops
on the first version that differs:

- `gcc-mingw-w64-x86-64-win32`, `gcc-mingw-w64-x86-64-posix`,
  `g++-mingw-w64-x86-64-win32`, `g++-mingw-w64-x86-64-posix`,
  `gcc-mingw-w64-i686-win32`, `gcc-mingw-w64-i686-posix` — `13.2.0-6ubuntu1+26.1`
- `mingw-w64-common`, `mingw-w64-x86-64-dev`, `mingw-w64-i686-dev` — `11.0.1-3build1`
- `binutils-mingw-w64-x86-64`, `binutils-mingw-w64-i686` —
  `2.41.90.20240122-1ubuntu1+11.4`

Every host package that contributes code to the training binaries is in that
list.

**Docker, and an account allowed to use it.** The 36 training binaries are
compiled on the host; rizin, `rz-sign` and the two Python steps run in throwaway
containers built from `Dockerfile`, each capped at 3 GB and 2 CPUs with no
network. Build that image first: it compiles rizin from its `v0.9.1` tag and
installs capstone 5.0.3 and pefile 2024.8.26, reaching docker.io, github.com and
pypi.org, and comes to about 670 MB. rizin publishes no 0.9.1 image, and the
recipe refuses any other version of the three, because capstone decides the
masks and rizin writes the file.

**`bash`, `curl`, `dpkg-deb`, `dpkg-query`, `sha256sum`.** No root. Nothing is
installed on the host: the 18 pinned `.debs` are unpacked under `OUT`.

**Network from the host** to deb.debian.org and snapshot.debian.org,
archive.ubuntu.com and launchpad.net. The first run downloads 239 MB of packages
and reuses them afterwards; a rerun also keeps the training binaries it already
built.

**2.1 GB of free space** for `OUT`, an empty directory outside your checkout,
and 3 GB of free memory while the containers run. A run from empty took 5 minutes
18 seconds on 12 cores. The binaries are compiled one after another, so fewer
cores take proportionally longer.

The run ends with `sha256sum -c expected.sha256` over the four files it wrote.

### What is pinned, and what is not

`recipe/debs.lock` pins 18 packages by file name and sha256 — GCC 12.2 with
mingw-w64 10 from Debian 12, GCC 14.2 with mingw-w64 12 from Debian 13, and the
i686 C++ compiler for GCC 13 from Ubuntu 24.04 — and the recipe refuses any file
whose hash differs, from the mirror or from the fallback. The rest of GCC 13 and
both binutils are the host's own packages, checked against the list above and
never fetched. The GCC 12 and 14 drivers call the host's binutils too (Debian
builds them with absolute `--with-as` and `--with-ld`), but every archive in
their link maps comes from the unpacked packages.

### What the claim is worth

The recipe reproduces both files byte for byte on a host set up as above.
Verified on 21 October 2026 on Ubuntu 24.04.4: all four sums matched, both with
an image kept from September and with one rebuilt from `Dockerfile` from scratch
the same day. The two images are not even the same size; their output is
identical.

It is reproducible for as long as that host set is installable. When Ubuntu
updates `gcc-mingw-w64` or `binutils-mingw-w64`, the recipe stops and names the
package that differs; it does not quietly write different files.
`ALLOW_HOST_DRIFT=1` builds anyway, and the result is a valid pack but not this
one, with different sums. The published sums belong to the package versions in
`debs.lock` and to no others. Nobody has yet rebuilt this on a second machine —
that is most of the reason the recipe is here.

The order of the training binaries is part of the output: when two builds give
the same signature, the first one's length is kept. To add a GCC version, pin
its packages in `debs.lock` under a new set, add its rows to `PLAN`, and append
its records file to the `pack` call after the others so the existing lines do not
change; then re-measure author false positives and recall on held-out files, and
update `expected.sha256`.

### Re-measure

```
OUT=/some/empty/dir eval/run.sh
```

`run.sh` runs the recipe into `OUT/recipe`, makes GCC 13-only packs at each
minimum length from the recipe's GCC 13 records, builds the 72 test files with
the recipe's toolchains, downloads Lua and SQLite from their project sites
against pinned sha256 sums, runs rizin on every stripped file with every pack in
throwaway containers without network, and prints the tables above. It took 15
minutes with `JOBS=3` and 2.4 GB under `OUT`, 2.1 GB of that the recipe's. A
rerun resumes where it stopped. It does not reproduce the two `Fc` counts; that
is said where those counts are.

