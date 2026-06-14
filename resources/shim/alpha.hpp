// alpha.hpp — Alpha complex (Delaunay-based) via CGAL. Needs GMP/MPFR for the
// exact kernels (bundled into libGudhi.a).
//
// The Alpha complex is a sparse, geometry-aware alternative to Rips: it has far
// fewer simplices and exact filtration values, ideal for low-dimensional point
// clouds. Output is a SimplexTree ready for persistence.

#ifndef GUDHI_SWIFT_ALPHA_HPP_
#define GUDHI_SWIFT_ALPHA_HPP_

#include "common.hpp"

namespace gudhi_swift {

/// Numerical precision for the Alpha complex.
enum class AlphaPrecision : int {
  fast = 0,   ///< CGAL::Epick_d — floating point, fastest, no exactness guarantee
  safe = 1,   ///< CGAL::Epeck_d, lazy — robust predicates (default)
  exact = 2,  ///< CGAL::Epeck_d, exact filtration values
};

/// Build an Alpha complex from a point cloud (row-major `rows`x`cols`).
/// @param maxAlphaSquare keep simplices with filtration <= this (squared α).
/// @param precision      see AlphaPrecision (default safe).
/// @param outputSquared  if true, filtration values are squared circumradii.
SimplexTree alphaComplex(const double* points, int rows, int cols,
                         double maxAlphaSquare, AlphaPrecision precision,
                         bool outputSquared);

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_ALPHA_HPP_
