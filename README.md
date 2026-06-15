# gudhi-xcframework-builder

Builds **`GudhiCore.xcframework`** — a macOS (arm64) static-library xcframework that
wraps a de-templated C++ facade over [GUDHI](https://gudhi.inria.fr) (Topological
Data Analysis), ready to consume from Swift with **C++ interop**.

It is the build half of a two-repo setup (mirroring `magentart-xcframework-builder`
→ `swift/MagentaRT`):

| repo | role |
|------|------|
| `cpp/gudhi-xcframework-builder` (this) | compiles the facade + GUDHI → `GudhiCore.xcframework` |
| `swift/SwiftGUDHI` | Swift package that imports the xcframework via `.interoperabilityMode(.Cxx)` |

## Why a facade?

GUDHI's core is header-only, C++17, and extremely template-heavy — its templates
can't be exposed to Swift directly. So we hand-write thin translation units under
`resources/shim/` that instantiate concrete GUDHI types and expose a tiny
pointer / POD / `std::vector` API. Only the facade headers are shipped in the
xcframework's `Headers/` (under the umbrella `gudhi_swift.hpp`); GUDHI's internals
stay hidden behind a pimpl.

## Bridged modules

| facade header | GUDHI functionality | deps |
|---|---|---|
| `common.hpp` | **SimplexTree** (insert/query/skeleton/star/cofaces) + **persistent cohomology** (diagram, Betti, intervals, pairs) + **edge collapse** | Boost |
| `rips.hpp` | **Rips** & **Sparse-Rips** (points or distance matrix) | Boost |
| `cubical.hpp` | **Bitmap cubical complex** (image persistence) | Boost |
| `witness.hpp` | **Witness** (table-based, combinatorial) | Boost |
| `distances.hpp` | **Bottleneck** + **Wasserstein** | Hera (bundled) |
| `subsampling.hpp` | farthest-point / random | Boost |
| `gudhi_mapper.hpp` | **Mapper** (nerve of a functional cover, 1-D + N-D lens) | Boost |

This `main` build is **permissive (MIT/BSD)** and links no CGAL/GMP/MPFR/Eigen.
The CGAL-backed modules (**Alpha, Tangential, Euclidean Witness, sparsify**) are
GPL-3.0 and live on the **`full-gpl`** branch — build there only if a GPL-3.0
distribution is acceptable. See [THIRD_PARTY_LICENSES.md](./THIRD_PARTY_LICENSES.md).

## License

The source in this repo is **MIT** ([LICENSE](./LICENSE)). The `main`-built
`GudhiCore.xcframework` links only permissive libraries (GUDHI MIT, Boost
BSL-1.0, Hera BSD) and is itself permissive — usable in closed-source apps. The
`full-gpl` branch's build is GPL-3.0-or-later. See
[THIRD_PARTY_LICENSES.md](./THIRD_PARTY_LICENSES.md).

## Upstream version

Pinned to GUDHI **`gudhi-release-3.12.0`** (v3.12.0). The canonical pin lives in
the committed [`GUDHI_VERSION`](./GUDHI_VERSION) file. `build.sh` verifies the
`GUDHI_SRC` checkout sits exactly on that tag (override with
`ALLOW_GUDHI_VERSION_MISMATCH=1`) and stamps it into:

- the facade's `version()` → e.g. `SwiftGUDHI facade 0.1 (GUDHI 3.12.0, tags/gudhi-release-3.12.0)`
- `Headers/GUDHI_PROVENANCE.txt` (shipped inside the xcframework) and
  `output/GudhiCore.xcframework.provenance.txt` (tag, version, commit).

**To upgrade GUDHI:** `git -C <gudhi-devel> checkout <new-tag>`, then bump
`GUDHI_VERSION`, then `make`.

## Prerequisites

- Xcode + command-line tools (`xcodebuild`, `clang++`, `libtool`).
- Homebrew **boost** (`brew install boost`). (The `full-gpl` branch additionally
  needs `cgal eigen gmp mpfr`.)
- A checkout of [`gudhi-devel`](https://github.com/GUDHI/gudhi-devel) with the
  **`ext/hera` submodule initialized** (bundled bottleneck/Wasserstein).

## Usage

```sh
cp config.sh.example config.sh   # edit GUDHI_SRC + (optional) mirror dir
make                             # or: ./build.sh
```

Output: `output/GudhiCore.xcframework` (+ `.zip` and `.zip.sha256` for a future
remote `binaryTarget`). If `SWIFT_PACKAGE_FRAMEWORKS_DIR` is set in `config.sh`,
the xcframework is also mirrored into the Swift package's `Frameworks/` dir.

`make clean` removes the assembled outputs; `make distclean` also drops the
staged GUDHI headers in `work/`.

## What `build.sh` does

1. **Stage** GUDHI's per-module `include/gudhi` trees into one flat include dir.
2. **Syntax-check + compile** every facade TU (`-std=c++17 -DNDEBUG`, GUDHI +
   CGAL + Eigen + Boost + Hera headers).
3. **Merge** all objects + `libgmp.a`/`libgmpxx.a`/`libmpfr.a` into `libGudhi.a`
   via `libtool -static` (so Swift links nothing extra).
4. **Assemble** `Headers/` (umbrella + per-module facade headers + `module.modulemap`
   + provenance).
5. **`xcodebuild -create-xcframework`** → `GudhiCore.xcframework`.
6. **Zip + checksum**.
7. **Optionally mirror** into the Swift package.

Internal (not shipped) headers live under `resources/shim/internal/`.

## API (facade)

Umbrella `resources/shim/gudhi_swift.hpp`, namespace `gudhi_swift`. See the
[bridged-modules table](#bridged-modules) above; the central type is
`SimplexTree` (built by the complex constructors, consumed by persistence), plus
free functions for distances, subsampling and Mapper.

## Roadmap

- **Flexible Mapper**: multi-dimensional lens + pluggable clustering (porting the
  pure-Python `MapperComplex` into the facade).
- **Extended persistence** + lower-star / flag generators.
- **iOS slices** + remote `url:`+`checksum:` distribution (factor out a shared
  `gmp-mpfr` xcframework if more CGAL packages appear).
