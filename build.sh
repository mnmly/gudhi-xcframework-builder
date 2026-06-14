#!/usr/bin/env bash
# build.sh — assemble Gudhi.xcframework (macOS arm64, static library) wrapping a
# de-templated C++ facade over GUDHI's Nerve / Graph-Induced-Complex (Mapper).
#
# Pipeline (idempotent):
#   1. Stage GUDHI's header-only library into a flat include/gudhi tree.
#   2. Syntax-check + compile the facade (GUDHI + Boost headers only).
#   3. Merge into a single static library (libtool).
#   4. Assemble the public Headers/ dir (facade header + module map).
#   5. xcodebuild -create-xcframework.
#   6. Zip + checksum (ready for a future remote binaryTarget).
#   7. Optionally mirror into the Swift package's Frameworks/ dir.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

# ── Config ───────────────────────────────────────────────────────────────────
if [ -f "$ROOT/config.sh" ]; then
  # shellcheck disable=SC1091
  source "$ROOT/config.sh"
else
  echo "[i] no config.sh found; using defaults from config.sh.example"
  # shellcheck disable=SC1091
  source "$ROOT/config.sh.example"
fi

: "${GUDHI_SRC:?set GUDHI_SRC to a gudhi-devel checkout}"
OUTPUT_DIR="${OUTPUT_DIR:-$ROOT/output}"
DEPLOYMENT_TARGET="${DEPLOYMENT_TARGET:-14.0}"
ARCHS="${ARCHS:-arm64}"
BOOST_INCLUDE="${BOOST_INCLUDE:-$(brew --prefix boost)/include}"
# CGAL-backed modules (Alpha/Delaunay, Tangential, Witness, Subsampling) need
# CGAL + Eigen headers and the compiled GMP/MPFR archives (for exact kernels).
# CGAL/Eigen/Boost/Hera are header-only and compile into the facade objects;
# only GMP/MPFR are binaries, and they are merged into libGudhi.a.
CGAL_INCLUDE="${CGAL_INCLUDE:-$(brew --prefix cgal)/include}"
EIGEN_INCLUDE="${EIGEN_INCLUDE:-$(brew --prefix eigen)/include/eigen3}"
GMP_PREFIX="${GMP_PREFIX:-$(brew --prefix gmp)}"
MPFR_PREFIX="${MPFR_PREFIX:-$(brew --prefix mpfr)}"
HERA_INCLUDE="${HERA_INCLUDE:-$GUDHI_SRC/ext/hera/include}"
# Canonical upstream pin. Env/config wins; otherwise the committed GUDHI_VERSION
# file is the source of truth.
GUDHI_TAG="${GUDHI_TAG:-$(cat "$ROOT/GUDHI_VERSION" 2>/dev/null | tr -d '[:space:]')}"
FACADE_VERSION="${FACADE_VERSION:-0.2}"

WORK="$ROOT/work"
STAGE="$WORK/stage"
GUDHI_INC="$STAGE/gudhi/include"
HEADERS="$STAGE/Headers"
SHIM_DIR="$ROOT/resources/shim"
UMBRELLA="gudhi_swift.hpp"          # the single Swift-facing umbrella header
MERGED="$STAGE/libGudhi.a"
XCFW="$OUTPUT_DIR/Gudhi.xcframework"

ARCH_FLAGS=()
for a in $ARCHS; do ARCH_FLAGS+=(-arch "$a"); done

# Header include set for compiling the facade TUs. CGAL/Eigen/Boost/Hera are
# header-only; GMP/MPFR contribute headers here and archives at the merge step.
INCLUDES=(
  -I "$GUDHI_INC"
  -I "$BOOST_INCLUDE"
  -I "$CGAL_INCLUDE"
  -I "$EIGEN_INCLUDE"
  -I "$GMP_PREFIX/include"
  -I "$MPFR_PREFIX/include"
  -I "$HERA_INCLUDE"
)
# Compiled archives merged into libGudhi.a so Swift links nothing extra.
DEP_ARCHIVES=(
  "$GMP_PREFIX/lib/libgmp.a"
  "$GMP_PREFIX/lib/libgmpxx.a"
  "$MPFR_PREFIX/lib/libmpfr.a"
)

echo "[i] GUDHI_SRC        = $GUDHI_SRC"
echo "[i] BOOST_INCLUDE    = $BOOST_INCLUDE"
echo "[i] CGAL_INCLUDE     = $CGAL_INCLUDE"
echo "[i] EIGEN_INCLUDE    = $EIGEN_INCLUDE"
echo "[i] GMP/MPFR         = $GMP_PREFIX | $MPFR_PREFIX"
echo "[i] HERA_INCLUDE     = $HERA_INCLUDE"
echo "[i] ARCHS            = $ARCHS"
echo "[i] DEPLOYMENT_TARGET= $DEPLOYMENT_TARGET"
echo "[i] OUTPUT_DIR       = $OUTPUT_DIR"

[ -d "$GUDHI_SRC/src/Nerve_GIC/include/gudhi" ] || {
  echo "[!] $GUDHI_SRC does not look like a gudhi-devel checkout (missing src/Nerve_GIC)"; exit 1; }
[ -d "$BOOST_INCLUDE/boost" ] || { echo "[!] Boost headers not found at $BOOST_INCLUDE"; exit 1; }
[ -f "$CGAL_INCLUDE/CGAL/version.h" ] || { echo "[!] CGAL headers not found at $CGAL_INCLUDE"; exit 1; }
[ -f "$EIGEN_INCLUDE/Eigen/Core" ] || { echo "[!] Eigen headers not found at $EIGEN_INCLUDE"; exit 1; }
[ -f "$HERA_INCLUDE/hera/bottleneck.h" ] || { echo "[!] Hera headers not found at $HERA_INCLUDE (init the ext/hera submodule)"; exit 1; }
for a in "${DEP_ARCHIVES[@]}"; do
  [ -f "$a" ] || { echo "[!] required static archive missing: $a"; exit 1; }
done

# ── Step 0: resolve + verify upstream GUDHI version ──────────────────────────
# Actual state of the checkout (works whether or not it is a git repo).
GUDHI_DESCRIBE="$(git -C "$GUDHI_SRC" describe --tags --always --dirty 2>/dev/null || echo unknown)"
GUDHI_REV="$(git -C "$GUDHI_SRC" rev-parse HEAD 2>/dev/null || echo unknown)"
GUDHI_VERSION="$(grep -m1 -E '^version' "$GUDHI_SRC/pyproject.toml" 2>/dev/null | sed -E 's/.*"([^"]+)".*/\1/')"
GUDHI_VERSION="${GUDHI_VERSION:-unknown}"

echo "[i] GUDHI_TAG (pin)  = ${GUDHI_TAG:-<none>}"
echo "[i] GUDHI checkout   = $GUDHI_DESCRIBE (v$GUDHI_VERSION, ${GUDHI_REV:0:12})"

# Verify the checkout sits exactly on the pinned tag (unless overridden). We
# match against the tags pointing at HEAD, normalizing an optional leading
# "tags/" (some clones nest tags under refs/tags/tags/...), so we don't depend
# on a particular ref path.
if [ -n "${GUDHI_TAG:-}" ]; then
  pin_norm="${GUDHI_TAG#tags/}"
  match=0
  while IFS= read -r t; do
    [ -z "$t" ] && continue
    [ "${t#tags/}" = "$pin_norm" ] && match=1
  done < <(git -C "$GUDHI_SRC" tag --points-at HEAD 2>/dev/null)

  if git -C "$GUDHI_SRC" rev-parse --git-dir >/dev/null 2>&1; then
    if [ "$match" != "1" ]; then
      echo "[!] version mismatch: GUDHI_SRC is at $GUDHI_DESCRIBE,"
      echo "    but the pin is GUDHI_TAG=$GUDHI_TAG."
      echo "    Fix:  git -C \"$GUDHI_SRC\" checkout $GUDHI_TAG"
      echo "    Or:   bump $ROOT/GUDHI_VERSION, or set ALLOW_GUDHI_VERSION_MISMATCH=1"
      [ "${ALLOW_GUDHI_VERSION_MISMATCH:-0}" = "1" ] || exit 1
      echo "[i] ALLOW_GUDHI_VERSION_MISMATCH=1 set — continuing despite mismatch"
    else
      echo "[i] verified: checkout is on pinned tag $GUDHI_TAG"
    fi
  else
    echo "[i] GUDHI_SRC is not a git repo — skipping strict tag match (v$GUDHI_VERSION)"
  fi
fi

# ── Step 1: stage GUDHI headers (flat include/gudhi) ─────────────────────────
echo "[1/7] staging GUDHI headers -> $GUDHI_INC/gudhi"
rm -rf "$GUDHI_INC"
mkdir -p "$GUDHI_INC/gudhi"
for moddir in "$GUDHI_SRC"/src/*/include/gudhi; do
  [ -d "$moddir" ] || continue
  cp -R "$moddir"/. "$GUDHI_INC/gudhi/"
done
echo "      staged $(find "$GUDHI_INC/gudhi" -name '*.h' | wc -l | tr -d ' ') headers"

# ── Step 2: compile every facade TU (GUDHI/CGAL/Eigen/Boost/Hera headers) ────
# Stamp the resolved upstream version into the facade so version() is traceable.
VERSION_DEFS=(
  -DGUDHI_SWIFT_FACADE_VERSION="\"$FACADE_VERSION\""
  -DGUDHI_SWIFT_GUDHI_VERSION="\"$GUDHI_VERSION\""
  -DGUDHI_SWIFT_GUDHI_DESCRIBE="\"$GUDHI_DESCRIBE\""
)
# Release build: -DNDEBUG disables CGAL/GUDHI/Hera internal asserts (correctness
# is unaffected; it's a large perf win and avoids over-strict debug checks, e.g.
# Hera's L-inf assert). -frounding-math gives CGAL exact kernels IEEE rounding.
CXXFLAGS=(-std=c++17 -O2 -DNDEBUG -fvisibility=default -frounding-math
          -mmacosx-version-min="$DEPLOYMENT_TARGET" "${ARCH_FLAGS[@]}")

mkdir -p "$STAGE/obj"
OBJECTS=()
shopt -s nullglob
TUS=("$SHIM_DIR"/*.cpp)
shopt -u nullglob
[ ${#TUS[@]} -gt 0 ] || { echo "[!] no .cpp translation units in $SHIM_DIR"; exit 1; }

echo "[2/7] syntax-checking ${#TUS[@]} facade TUs"
for tu in "${TUS[@]}"; do
  clang++ "${CXXFLAGS[@]}" -fsyntax-only "${VERSION_DEFS[@]}" "${INCLUDES[@]}" "$tu"
done

echo "[2/7] compiling ${#TUS[@]} facade TUs"
for tu in "${TUS[@]}"; do
  obj="$STAGE/obj/$(basename "${tu%.cpp}").o"
  echo "      cc $(basename "$tu")"
  clang++ "${CXXFLAGS[@]}" "${VERSION_DEFS[@]}" "${INCLUDES[@]}" -c "$tu" -o "$obj"
  OBJECTS+=("$obj")
done

# ── Step 3: merge facade objects + GMP/MPFR archives into one static library ──
echo "[3/7] merging static library -> $MERGED"
rm -f "$MERGED"
libtool -static -o "$MERGED" "${OBJECTS[@]}" "${DEP_ARCHIVES[@]}"

# ── Step 4: assemble public Headers/ + provenance ────────────────────────────
echo "[4/7] assembling Headers/"
rm -rf "$HEADERS"
mkdir -p "$HEADERS"
# Ship every facade header (the umbrella + per-module headers) + the module map.
cp "$SHIM_DIR"/*.hpp "$HEADERS/"
cp "$ROOT/resources/module.modulemap" "$HEADERS/module.modulemap"

# Provenance record: exactly which upstream this binary was built from. Shipped
# inside the xcframework's Headers/ and alongside the output zip.
PROVENANCE="$HEADERS/GUDHI_PROVENANCE.txt"
cat > "$PROVENANCE" <<EOF
SwiftGUDHI facade $FACADE_VERSION
upstream:   GUDHI (https://github.com/GUDHI/gudhi-devel)
pinned tag: ${GUDHI_TAG:-<none>}
version:    $GUDHI_VERSION
git:        $GUDHI_DESCRIBE
commit:     $GUDHI_REV
source:     $GUDHI_SRC
EOF
cat "$PROVENANCE" | sed 's/^/      /'

# ── Step 5: create the xcframework ───────────────────────────────────────────
echo "[5/7] creating xcframework -> $XCFW"
mkdir -p "$OUTPUT_DIR"
rm -rf "$XCFW"
xcodebuild -create-xcframework \
  -library "$MERGED" -headers "$HEADERS" \
  -output "$XCFW"

# ── Step 6: zip + checksum + provenance ──────────────────────────────────────
echo "[6/7] packaging"
cp "$PROVENANCE" "$OUTPUT_DIR/Gudhi.xcframework.provenance.txt"
( cd "$OUTPUT_DIR" && \
  ditto -c -k --keepParent "Gudhi.xcframework" "Gudhi.xcframework.zip" && \
  shasum -a 256 "Gudhi.xcframework.zip" | tee "Gudhi.xcframework.zip.sha256" )

# ── Step 7: optional mirror into the Swift package ───────────────────────────
if [ -n "${SWIFT_PACKAGE_FRAMEWORKS_DIR:-}" ] && [ -d "$(dirname "$SWIFT_PACKAGE_FRAMEWORKS_DIR")" ]; then
  echo "[7/7] mirroring xcframework -> $SWIFT_PACKAGE_FRAMEWORKS_DIR"
  mkdir -p "$SWIFT_PACKAGE_FRAMEWORKS_DIR"
  rm -rf "$SWIFT_PACKAGE_FRAMEWORKS_DIR/Gudhi.xcframework"
  ditto "$XCFW" "$SWIFT_PACKAGE_FRAMEWORKS_DIR/Gudhi.xcframework"
else
  echo "[7/7] skipping Swift-package mirror (SWIFT_PACKAGE_FRAMEWORKS_DIR unset or parent missing)"
fi

echo "[✓] done: $XCFW"
