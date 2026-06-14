// tangential.hpp — Tangential complex (manifold reconstruction, CGAL).

#ifndef GUDHI_SWIFT_TANGENTIAL_HPP_
#define GUDHI_SWIFT_TANGENTIAL_HPP_

#include "common.hpp"

namespace gudhi_swift {

/// Build a Tangential complex from a point cloud sampled from a manifold of
/// the given intrinsic dimension, then export it as a SimplexTree.
/// (Inconsistencies, if any, are left as-is; this is the basic construction.)
SimplexTree tangentialComplex(const double* points, int rows, int cols, int intrinsicDimension);

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_TANGENTIAL_HPP_
