// distances.hpp — distances between persistence diagrams (bundled Hera).
//
// Diagrams are passed as flat row-major arrays of length 2*n: [b0,d0,b1,d1,...].
// A `death` of +inf marks an essential feature; Hera handles infinite points.

#ifndef GUDHI_SWIFT_DISTANCES_HPP_
#define GUDHI_SWIFT_DISTANCES_HPP_

namespace gudhi_swift {

/// Bottleneck distance between two diagrams. `delta` is the relative error
/// (0 => exact). `n1`/`n2` are the point counts (arrays have 2*n entries).
double bottleneckDistance(const double* diagram1, int n1,
                          const double* diagram2, int n2,
                          double delta);

/// Wasserstein distance between two diagrams.
/// @param order        the exponent p of W_p (e.g. 1 or 2).
/// @param internalP    the ground metric on R^2 (use a negative value for L-inf).
/// @param delta        relative error of the approximation.
double wassersteinDistance(const double* diagram1, int n1,
                           const double* diagram2, int n2,
                           double order, double internalP, double delta);

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_DISTANCES_HPP_
