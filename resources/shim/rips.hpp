// rips.hpp — Rips / Sparse-Rips complex builders (no CGAL needed).
//
// A Rips complex connects points closer than a threshold and expands the flag
// complex up to a dimension. The output is a SimplexTree ready for persistence.

#ifndef GUDHI_SWIFT_RIPS_HPP_
#define GUDHI_SWIFT_RIPS_HPP_

#include "common.hpp"

namespace gudhi_swift {

/// Rips complex from a point cloud (row-major `rows`x`cols`), Euclidean metric.
SimplexTree ripsComplexFromPoints(const double* points, int rows, int cols,
                                  double maxEdgeLength, int maxDimension);

/// Rips complex from a precomputed `n`x`n` distance matrix (row-major).
SimplexTree ripsComplexFromDistanceMatrix(const double* distances, int n,
                                          double maxEdgeLength, int maxDimension);

/// Sparse (approximate) Rips from points. `epsilon` is the approximation factor.
SimplexTree sparseRipsFromPoints(const double* points, int rows, int cols,
                                 double epsilon, double maxEdgeLength, int maxDimension);

/// Sparse (approximate) Rips from a distance matrix.
SimplexTree sparseRipsFromDistanceMatrix(const double* distances, int n,
                                         double epsilon, double maxEdgeLength, int maxDimension);

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_RIPS_HPP_
