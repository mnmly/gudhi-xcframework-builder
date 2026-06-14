// subsampling.hpp — point-cloud subsampling (landmark selection / sparsification).

#ifndef GUDHI_SWIFT_SUBSAMPLING_HPP_
#define GUDHI_SWIFT_SUBSAMPLING_HPP_

#include "common.hpp"

namespace gudhi_swift {

/// Greedy farthest-point sampling: pick `nbPoints` spread-out landmarks.
/// @param startingPoint index of the first landmark; negative => random.
/// @param metric if true, use the triangle-inequality-accelerated variant.
DoubleMatrix chooseNFarthestPoints(const double* points, int rows, int cols,
                                   int nbPoints, int startingPoint, bool metric);

/// Pick `nbPoints` points uniformly at random.
DoubleMatrix pickNRandomPoints(const double* points, int rows, int cols, int nbPoints);

/// Keep points no two of which are closer than sqrt(`minSquaredDist`) (CGAL).
DoubleMatrix sparsifyPointSet(const double* points, int rows, int cols, double minSquaredDist);

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_SUBSAMPLING_HPP_
