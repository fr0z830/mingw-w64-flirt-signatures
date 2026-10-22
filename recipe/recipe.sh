#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Angelos Katis (RADECTI)
# SPDX-License-Identifier: MIT
# Rebuild the mingw-w64 runtime FLIRT pack for rizin (GCC 12 + 13 + 14,
# x86 and x64) from source, and check it against expected.sha256. See README.md.
#
#   OUT=/some/empty/dir ./recipe.sh        # OUT is required; keep it outside the repo
#
# Needs: bash, curl, dpkg-deb, dpkg-query, sha256sum, docker, and the host packages
# listed as `host` in debs.lock. Nothing is installed: the pinned .debs are unpacked
# under $OUT/tc. rizin, rz-sign and the Python steps run in throwaway containers of
# $IMAGE with no network.
#
# Env: IMAGE (any image with rizin 0.9.1, capstone 5.0.3, pefile 2024.8.26;
#      no default — pass one), ALLOW_HOST_DRIFT=1 (build
# although a host package differs from debs.lock), FETCH_FALLBACK_ONLY=1 (skip the
# primary mirrors; exercises snapshot.debian.org / launchpad), VERIFY=0 (skip the
# final hash check).
# Output: $OUT/mingw-w64-gcc12-14_x86_{32,64}.{sig,pat,manifest.json}
set -euo pipefail
export LC_ALL=C
umask 022
R=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:?set OUT to a directory outside the repo (about 2.1 GB)}
# No default on purpose: the pack must be rebuildable without our image.
# Any image carrying rizin 0.9.1, capstone 5.0.3 and pefile 2024.8.26 works.
: "${IMAGE:?set IMAGE to an image with rizin 0.9.1, capstone 5.0.3, pefile 2024.8.26}"
MIN_EXT=48
S=$R/src
LOCK=$R/debs.lock
case $OUT in *:*) echo "OUT must not contain ':'"; exit 2 ;; esac
mkdir -p "$OUT"/{bin,obj,tc,debs,records,tmp}
OUT=$(cd "$OUT" && pwd)
export TMPDIR=$OUT/tmp
lock() { grep -vE '^[[:space:]]*(#|$)' "$LOCK"; }

# ---- 1. host toolchain (GCC 13 and binutils) must match the lock --------------
drift=0
while read -r set pkg ver _; do
  if [ "$set" != host ]; then continue; fi
  have=$(dpkg-query -W -f='${Version}' "$pkg" 2>/dev/null || echo missing)
  if [ "$have" != "$ver" ]; then
    echo "host package $pkg is '$have', debs.lock pins '$ver'"; drift=1
  fi
done < <(lock)
if [ $drift = 1 ] && [ "${ALLOW_HOST_DRIFT:-0}" != 1 ]; then
  echo "refusing to build: the pack would not be the evaluated one (ALLOW_HOST_DRIFT=1 overrides)"; exit 2
fi

# ---- 2. the image: rizin 0.9.1 writes the .sig; capstone decides the masks ----
want="0.9.1 5.0.3 2024.8.26 "
got=$(docker run --rm --network none --entrypoint sh "$IMAGE" -c \
  'rizin -v | head -1 | cut -d" " -f2; python3 -c "import capstone, pefile; print(capstone.__version__, pefile.__version__)"' \
  | tr '\n' ' ')
if [ "$got" != "$want" ]; then
  echo "image $IMAGE has rizin/capstone/pefile '$got', expected '$want'"; exit 2
fi

# ---- 3. fetch, verify, extract -------------------------------------------------
fetch() {  # set sha256 filename pooldir sha1
  local set=$1 sum=$2 file=$3 pool=$4 sha1=$5 dst=$OUT/debs/$3 enc u
  enc=$(printf '%s' "$file" | sed 's/+/%2B/g')
  if [ -f "$dst" ] && echo "$sum  $dst" | sha256sum -c --status; then return 0; fi
  rm -f "$dst"
  local urls=()
  if [ "$set" = noble13 ]; then
    if [ "${FETCH_FALLBACK_ONLY:-0}" != 1 ]; then urls+=("https://archive.ubuntu.com/ubuntu/$pool/$enc"); fi
    urls+=("https://launchpad.net/ubuntu/+archive/primary/+files/$enc")
  else
    if [ "${FETCH_FALLBACK_ONLY:-0}" != 1 ]; then urls+=("https://deb.debian.org/debian/$pool/$enc"); fi
    urls+=("https://snapshot.debian.org/file/$sha1")
  fi
  for u in "${urls[@]}"; do
    if curl -fsSL --retry 3 -m 900 -o "$dst.part" "$u" && echo "$sum  $dst.part" | sha256sum -c --status; then
      mv "$dst.part" "$dst"; echo "fetched $file from $u"; return 0
    fi
    echo "  $u: download failed or sha256 differs" >&2; rm -f "$dst.part"
  done
  echo "cannot fetch $file with sha256 $sum"; exit 4
}
while read -r set sum file pool sha1; do
  if [ "$set" != host ]; then fetch "$set" "$sum" "$file" "$pool" "$sha1"; fi
done < <(lock)
for set in deb12 deb14 noble13; do
  T=$OUT/tc/$set
  if [ -f "$T/.extracted" ]; then continue; fi
  rm -rf "$T"; mkdir -p "$T"
  if [ $set = noble13 ]; then   # the host's i686 C compiler and libgcc, plus the pinned i686 g++
    mkdir -p "$T/usr/lib/gcc/i686-w64-mingw32"
    cp -a /usr/lib/gcc/i686-w64-mingw32/13-win32 /usr/lib/gcc/i686-w64-mingw32/13-posix \
          "$T/usr/lib/gcc/i686-w64-mingw32/"
    ln -sfn /usr/i686-w64-mingw32 "$T/usr/i686-w64-mingw32"
  fi
  while read -r s _ file _ _; do
    if [ "$s" = $set ]; then dpkg-deb -x "$OUT/debs/$file" "$T"; fi
  done < <(lock)
  touch "$T/.extracted"
done

# ---- 4. training binaries (unstripped, with linker maps) ----------------------
driver() {  # profile bits lang -> compiler driver
  local p=$1 b=$2 l=$3 t fl tool
  t=x86_64; if [ "$b" = 32 ]; then t=i686; fi
  tool=gcc; if [ "$l" = cxx ]; then tool=g++; fi
  case $p in
    n13w|n13p)
      fl=win32; if [ $p = n13p ]; then fl=posix; fi
      if [ "$tool" = g++ ] && [ "$t" = i686 ]; then echo "$OUT/tc/noble13/usr/bin/i686-w64-mingw32-g++-$fl"
      else echo "$t-w64-mingw32-$tool-$fl"; fi ;;
    deb12|deb14) echo "$OUT/tc/$p/usr/bin/$t-w64-mingw32-$tool-win32" ;;
  esac
}
# One row per binary: name profile bits lang kind source cflags | ldflags.
# THE ORDER IS PART OF THE OUTPUT (see lib/patgen.py): GCC 13, then 12, then 14,
# and inside each version the order below.
# -D_FORTIFY_SOURCE=2 is GCC 13 only: with mingw-w64 10 headers (GCC 12) it breaks
# compiling tr_c.c, which takes printf's address; GCC 14 was built like GCC 12.
PLAN='
tr_c_32_n13p       n13p  32 c   exe tr_c.c      -O2 | -static -lpthread
tr_c_32_n13w       n13w  32 c   exe tr_c.c      -O2 | -static -lpthread
tr_c_64_n13p       n13p  64 c   exe tr_c.c      -O2 | -static -lpthread
tr_c_64_n13w       n13w  64 c   exe tr_c.c      -O2 | -static -lpthread
tr_c_ssp_32_n13w   n13w  32 c   exe tr_c.c      -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 | -static -lssp -lpthread
tr_c_ssp_64_n13w   n13w  64 c   exe tr_c.c      -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 | -static -lssp -lpthread
tr_cpp_32_n13p     n13p  32 cxx exe tr_cpp.cpp  -std=gnu++17 -O2 | -static
tr_cpp_32_n13w     n13w  32 cxx exe tr_cpp.cpp  -std=gnu++17 -O2 | -static
tr_cpp_64_n13p     n13p  64 cxx exe tr_cpp.cpp  -std=gnu++17 -O2 | -static
tr_cpp_64_n13w     n13w  64 cxx exe tr_cpp.cpp  -std=gnu++17 -O2 | -static
tr_cpp_O0_32_n13w  n13w  32 cxx exe tr_cpp.cpp  -std=gnu++17 -O0 | -static
tr_cpp_O0_64_n13w  n13w  64 cxx exe tr_cpp.cpp  -std=gnu++17 -O0 | -static
tr_dll_32_n13w     n13w  32 c   dll tr_dll.c    -O2 | -shared -static-libgcc
tr_dll_64_n13w     n13w  64 c   dll tr_dll.c    -O2 | -shared -static-libgcc
tr_wmain_32_n13w   n13w  32 c   exe tr_wmain.c  -O2 -municode | -municode -static
tr_wmain_64_n13w   n13w  64 c   exe tr_wmain.c  -O2 -municode | -municode -static
tr_c_32_deb12      deb12 32 c   exe tr_c.c      -O2 | -static -lpthread
tr_c_64_deb12      deb12 64 c   exe tr_c.c      -O2 | -static -lpthread
tr_c_ssp_32_deb12  deb12 32 c   exe tr_c.c      -O2 -fstack-protector-strong | -static -lssp -lpthread
tr_c_ssp_64_deb12  deb12 64 c   exe tr_c.c      -O2 -fstack-protector-strong | -static -lssp -lpthread
tr_cpp_32_deb12    deb12 32 cxx exe tr_cpp.cpp  -std=gnu++17 -O2 | -static
tr_cpp_64_deb12    deb12 64 cxx exe tr_cpp.cpp  -std=gnu++17 -O2 | -static
tr_dll_32_deb12    deb12 32 c   dll tr_dll.c    -O2 | -shared -static-libgcc
tr_dll_64_deb12    deb12 64 c   dll tr_dll.c    -O2 | -shared -static-libgcc
tr_wmain_32_deb12  deb12 32 c   exe tr_wmain.c  -O2 -municode | -municode -static
tr_wmain_64_deb12  deb12 64 c   exe tr_wmain.c  -O2 -municode | -municode -static
tr_c_32_deb14      deb14 32 c   exe tr_c.c      -O2 | -static -lpthread
tr_c_64_deb14      deb14 64 c   exe tr_c.c      -O2 | -static -lpthread
tr_c_ssp_32_deb14  deb14 32 c   exe tr_c.c      -O2 -fstack-protector-strong | -static -lssp -lpthread
tr_c_ssp_64_deb14  deb14 64 c   exe tr_c.c      -O2 -fstack-protector-strong | -static -lssp -lpthread
tr_cpp_32_deb14    deb14 32 cxx exe tr_cpp.cpp  -std=gnu++17 -O2 | -static
tr_cpp_64_deb14    deb14 64 cxx exe tr_cpp.cpp  -std=gnu++17 -O2 | -static
tr_dll_32_deb14    deb14 32 c   dll tr_dll.c    -O2 | -shared -static-libgcc
tr_dll_64_deb14    deb14 64 c   dll tr_dll.c    -O2 | -shared -static-libgcc
tr_wmain_32_deb14  deb14 32 c   exe tr_wmain.c  -O2 -municode | -municode -static
tr_wmain_64_deb14  deb14 64 c   exe tr_wmain.c  -O2 -municode | -municode -static
'
declare -A SPECS=([n13]="" [deb12]="" [deb14]="")
while read -r name prof bits lang kind src rest; do
  if [ -z "$name" ]; then continue; fi
  cflags=${rest%%|*}; ldflags=${rest#*|}
  SPECS[${prof%[pw]}]+="$name:/o/bin/$name:/o/bin/$name.map:$OUT/obj/$name "
  if [ -f "$OUT/bin/$name.map" ]; then continue; fi
  drv=$(driver "$prof" "$bits" "$lang")
  rm -rf "${OUT:?}/obj/$name"; mkdir -p "$OUT/obj/$name"
  obj=$OUT/obj/$name/${src%.*}.o
  # shellcheck disable=SC2086
  "$drv" $cflags -c "$S/$src" -o "$obj"
  out=$OUT/bin/$name
  if [ "$kind" = dll ]; then out=$out.dll; fi
  # shellcheck disable=SC2086
  "$drv" "$obj" $ldflags -o "$out" -Wl,-Map="$OUT/bin/$name.map"
  if [ ! -f "$out" ]; then mv "$out.exe" "$out"; fi
  if [ "$kind" = dll ]; then mv "$out" "$OUT/bin/$name"; fi
  echo "built $name"
done <<< "$PLAN"

# ---- 5. records (runtime functions only), merge 13 -> 12 -> 14, .sig ----------
chmod a+rwx "$OUT" "$OUT/records"
run() { docker run --rm --network none --memory 3g --cpus 2 -e PYTHONDONTWRITEBYTECODE=1 \
          -v "$R/lib":/r:ro -v "$OUT":/o --entrypoint sh "$IMAGE" -c "$1"; }
for v in n13 deb12 deb14; do
  run "python3 /r/patgen.py records /o/records/$v.jsonl ${SPECS[$v]}"
done
# flirt.node.optimize=1 keeps the tail bytes (rz-sign's default, 2, drops them).
# flirt.sig.library stays at rizin's default, so the bytes match expected.sha256.
run "set -e; for B in 32 64; do
       python3 /r/patgen.py pack --bits \$B --min-ext $MIN_EXT --out /o/mingw-w64-gcc12-14_x86_\$B.pat \
         /o/records/n13.jsonl /o/records/deb12.jsonl /o/records/deb14.jsonl
       rz-sign -q -e flirt.node.optimize=1 -c /o/mingw-w64-gcc12-14_x86_\$B.sig /o/mingw-w64-gcc12-14_x86_\$B.pat
     done"

# ---- 6. same bytes as the evaluated pack? --------------------------------------
ls -la "$OUT"/mingw-w64-gcc12-14_x86_*.sig
if [ "${VERIFY:-1}" = 1 ]; then
  if ! (cd "$OUT" && sha256sum -c "$R/expected.sha256"); then echo "MISMATCH with expected.sha256"; exit 3; fi
fi
