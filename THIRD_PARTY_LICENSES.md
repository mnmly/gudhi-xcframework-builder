# Third-party licenses

`GudhiCore.xcframework` (the `main`/permissive build) statically links the
following libraries. All are permissive (MIT / BSD / BSL / MPL), so the binary —
and any application that links it — has **no copyleft obligations** and may be
used in closed-source / commercial / App Store apps, subject to the usual notice
requirements below.

| Component | License (SPDX) | Notes |
|---|---|---|
| SwiftGUDHI facade (this project) | MIT | original glue / wrapper code |
| [GUDHI](https://gudhi.inria.fr) 3.12.0 | MIT | TDA algorithms (header-only, compiled in) |
| [Boost](https://www.boost.org) | BSL-1.0 | graph / utilities (header-only) |
| [Hera](https://github.com/grey-narn/hera) | BSD | bottleneck / Wasserstein (bundled in GUDHI `ext/hera`) |

This permissive build **excludes** the CGAL-backed modules (Alpha, Tangential,
Euclidean Witness, sparsify). CGAL's triangulation packages are GPL-3.0-or-later,
which would make the whole binary GPL-3.0. Those modules — and the resulting
GPL-3.0 build that also links CGAL (LGPL/GPL), GMP & MPFR (LGPL-3.0) and Eigen
(MPL-2.0) — live on the **`full-gpl`** branch. Build from that branch only if a
GPL-3.0 distribution is acceptable for your use.

## Notice requirements

Redistributing the binary should preserve the copyright/permission notices of
the components above (GUDHI MIT, Boost BSL-1.0, Hera BSD). Their full texts are
available in their respective upstream repositories.
