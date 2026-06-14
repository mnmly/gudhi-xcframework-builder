// witness.hpp — Witness complexes (landmark-based reconstruction).

#ifndef GUDHI_SWIFT_WITNESS_HPP_
#define GUDHI_SWIFT_WITNESS_HPP_

#include "common.hpp"

namespace gudhi_swift {

/// Euclidean witness complex from a landmark set and a witness (point) set.
/// @param maxAlphaSquare relaxation parameter (squared).
/// @param limitDimension max simplex dimension (negative => unlimited).
SimplexTree euclideanWitnessComplex(const double* landmarks, int numLandmarks, int landmarkDim,
                                    const double* witnesses, int numWitnesses, int witnessDim,
                                    double maxAlphaSquare, int limitDimension);

/// Witness complex from a precomputed nearest-landmark table: for each of
/// `numWitnesses` witnesses, its `k` nearest landmarks given as indices
/// (`landmarkIndices`, length numWitnesses*k) and squared distances
/// (`squaredDistances`, same length), nearest-first.
SimplexTree witnessComplexFromTable(const int* landmarkIndices, const double* squaredDistances,
                                    int numWitnesses, int k,
                                    double maxAlphaSquare, int limitDimension);

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_WITNESS_HPP_
