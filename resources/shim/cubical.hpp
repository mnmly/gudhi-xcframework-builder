// cubical.hpp — Bitmap cubical complex (persistence of images / grids).
//
// A cubical complex is built from a regular grid of filtration values (e.g.
// pixel/voxel intensities). It is the standard tool for image persistence.

#ifndef GUDHI_SWIFT_CUBICAL_HPP_
#define GUDHI_SWIFT_CUBICAL_HPP_

#include <memory>
#include <vector>

#include "common.hpp"

namespace gudhi_swift {

/// A bitmap cubical complex over a regular grid.
class CubicalComplex {
 public:
  CubicalComplex();

  int numSimplices() const;
  int dimension() const;

  /// Compute and return the persistence diagram.
  std::vector<PersistenceInterval> persistence(int homologyCoeffField = 11,
                                               double minPersistence = 0.0) const;
  std::vector<int> bettiNumbers() const;
  std::vector<BirthDeath> persistenceIntervalsInDimension(int dimension) const;

  struct Impl;
  std::shared_ptr<Impl> impl;
};

/// Build a cubical complex from a regular grid.
/// @param dimensions   grid shape (length `ndims`).
/// @param cells        filtration values (length `ncells`).
/// @param inputTopCells if true, `cells` are top-dimensional cell values;
///                      otherwise they are vertex values.
CubicalComplex cubicalComplex(const int* dimensions, int ndims,
                              const double* cells, int ncells,
                              bool inputTopCells);

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_CUBICAL_HPP_
