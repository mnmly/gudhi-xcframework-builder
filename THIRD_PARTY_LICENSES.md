# Third-party licenses — full (GPL-3.0) build

`GudhiCoreFull.xcframework` (this `full-gpl` branch) statically links the
following. Because CGAL's triangulation packages are GPL-3.0-or-later, the
**binary as a whole is GPL-3.0-or-later** — any app linking it must comply with
the GPL (GPL-compatible, source available; no closed-source / App Store).

| Component | License (SPDX) | Notes |
|---|---|---|
| SwiftGUDHIFull facade (this project) | MIT | original glue / wrapper code |
| [GUDHI](https://gudhi.inria.fr) 3.12.0 | MIT | TDA algorithms (header-only) |
| [Boost](https://www.boost.org) | BSL-1.0 | graph / utilities (header-only) |
| [Hera](https://github.com/grey-narn/hera) | BSD | bottleneck / Wasserstein (bundled) |
| [CGAL](https://www.cgal.org) kernels (Epeck_d/Epick_d) | LGPL-3.0-or-later | exact / inexact kernels |
| **CGAL dD Triangulation** | **GPL-3.0-or-later** | Alpha, Tangential, Euclidean Witness ⇒ makes the binary GPL |
| [Eigen](https://eigen.tuxfamily.org) | MPL-2.0 | linear algebra (via CGAL) |
| [GMP](https://gmplib.org) (libgmp/libgmpxx) | LGPL-3.0+ (or GPL-2.0+) | exact arithmetic (static-linked) |
| [MPFR](https://www.mpfr.org) | LGPL-3.0+ | exact arithmetic (static-linked) |

**Effective license of the binary: GPL-3.0-or-later** (see LICENSE.GPL-3.0).

For a permissive MIT/BSD binary (no CGAL: drops Alpha/Tangential/Euclidean
Witness/sparsify), use the `main` branch / `GudhiCore.xcframework`.
