// witness.hpp — table-based witness complex (landmark-based reconstruction).
//
// Combinatorial only (no CGAL): the caller supplies a precomputed
// nearest-landmark table. The CGAL-backed Euclidean witness complex (which
// computes the table itself) is intentionally omitted from this permissive
// build — compute the table externally (e.g. with a kd-tree) and pass it here.

#ifndef GUDHI_SWIFT_WITNESS_HPP_
#define GUDHI_SWIFT_WITNESS_HPP_

#include "common.hpp"

namespace gudhi_swift {

/// Witness complex from a precomputed nearest-landmark table: for each of
/// `numWitnesses` witnesses, its `k` nearest landmarks given as indices
/// (`landmarkIndices`, length numWitnesses*k) and squared distances
/// (`squaredDistances`, same length), nearest-first.
/// @param maxAlphaSquare relaxation parameter (squared).
/// @param limitDimension max simplex dimension (negative => unlimited).
SimplexTree witnessComplexFromTable(const int* landmarkIndices, const double* squaredDistances,
                                    int numWitnesses, int k,
                                    double maxAlphaSquare, int limitDimension);

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_WITNESS_HPP_
