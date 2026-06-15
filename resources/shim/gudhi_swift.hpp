// gudhi_swift.hpp — umbrella facade header (the module's single entry point).
//
// Includes every per-module facade header so Swift imports one Clang module
// (`GudhiCore`) covering all bridged GUDHI functionality. Each included header
// uses only std / POD types in its public signatures; no GUDHI/CGAL internals
// are exposed to Swift.

#ifndef GUDHI_SWIFT_UMBRELLA_HPP_
#define GUDHI_SWIFT_UMBRELLA_HPP_

#include "common.hpp"        // SimplexTree + persistence (the hub)
#include "rips.hpp"          // Rips / Sparse-Rips complexes
#include "cubical.hpp"       // Bitmap cubical complex (image persistence)
#include "witness.hpp"       // Witness complex (table-based, combinatorial)
#include "distances.hpp"     // bottleneck / Wasserstein (Hera)
#include "subsampling.hpp"   // farthest-point / random
#include "gudhi_mapper.hpp"  // Mapper (Nerve of a functional cover)

#include <string>

namespace gudhi_swift {
/// Library version string, stamped from the pinned upstream GUDHI tag.
std::string version();
}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_UMBRELLA_HPP_
