// common.hpp — shared Swift-facing types + the central SimplexTree object.
//
// SimplexTree is the hub of GUDHI: complex builders (Rips, Alpha, Witness, ...)
// produce one, and persistence is computed on it. It is exposed as a *value
// type backed by a shared_ptr*, so Swift sees an ergonomic struct with
// reference semantics (copies share the same underlying complex). All query and
// mutation methods are `const` (they mutate through the shared_ptr), so Swift
// imports them as plain methods rather than `mutating` ones.
//
// Public signatures use only std / POD types — no GUDHI headers leak here.

#ifndef GUDHI_SWIFT_COMMON_HPP_
#define GUDHI_SWIFT_COMMON_HPP_

#include <memory>
#include <utility>
#include <vector>

namespace gudhi_swift {

// `using` aliases make these std::vector specializations nameable (and thus
// constructible / appendable / readable) from Swift via CxxStdlib.
using IntVector = std::vector<int>;
using DoubleVector = std::vector<double>;
using DoubleMatrix = std::vector<std::vector<double>>;

/// A simplex (sorted vertex ids) together with its filtration value.
struct FilteredSimplex {
  std::vector<int> vertices;
  double filtration = 0.0;
};

/// A point of a persistence diagram. `death` is +inf for essential features.
struct PersistenceInterval {
  int dimension = 0;
  double birth = 0.0;
  double death = 0.0;
};

/// A persistence pair as simplices: the birth simplex and the death simplex
/// (death empty for an essential/never-dying feature).
struct PersistencePair {
  std::vector<int> birth;
  std::vector<int> death;
};

/// A (birth, death) interval in a fixed homology dimension.
struct BirthDeath {
  double birth = 0.0;
  double death = 0.0;
};

/// GUDHI's Simplex_tree, de-templated for Swift. Reference semantics via a
/// shared backing store.
class SimplexTree {
 public:
  SimplexTree();  // empty complex

  // ── construction / mutation ────────────────────────────────────────────────
  /// Insert a simplex and all its subfaces. Returns true if it was new.
  bool insert(const std::vector<int>& simplex, double filtration = 0.0) const;
  /// Insert many simplices at once (each with its filtration). Convenience.
  void insertSimplices(const std::vector<FilteredSimplex>& simplices) const;
  /// Assign a filtration value to an existing simplex (may break monotonicity).
  void assignFiltration(const std::vector<int>& simplex, double filtration) const;
  /// Raise filtration values so the complex is a valid filtration. Returns true if changed.
  bool makeFiltrationNonDecreasing() const;
  /// Remove simplices with filtration above the threshold. Returns true if changed.
  bool pruneAboveFiltration(double filtration) const;
  /// Remove simplices of dimension above the threshold. Returns true if changed.
  bool pruneAboveDimension(int dimension) const;
  /// Expand a flag (1-skeleton) complex up to `maxDimension`.
  void expansion(int maxDimension) const;
  /// Collapse edges of a flag complex `nbIterations` times. Reduces the edge
  /// count while preserving persistent homology (call before `expansion`).
  void collapseEdges(int nbIterations) const;
  /// Reset every filtration value (for simplices of dim >= minDimension).
  void resetFiltration(double filtration, int minDimension = 0) const;
  /// Empty the complex.
  void clear() const;

  // ── queries ────────────────────────────────────────────────────────────────
  int numVertices() const;
  int numSimplices() const;
  int dimension() const;
  bool find(const std::vector<int>& simplex) const;
  double filtration(const std::vector<int>& simplex) const;

  std::vector<FilteredSimplex> getSimplices() const;           // all simplices
  std::vector<FilteredSimplex> getSkeleton(int dimension) const;
  std::vector<FilteredSimplex> getStar(const std::vector<int>& simplex) const;
  std::vector<FilteredSimplex> getCofaces(const std::vector<int>& simplex, int codimension) const;
  std::vector<FilteredSimplex> getBoundaries(const std::vector<int>& simplex) const;

  // ── persistence ────────────────────────────────────────────────────────────
  /// Compute persistent cohomology (over Z/pZ). Call before the accessors below.
  /// `homologyCoeffField` must be prime (default 11). `persistenceDimMax`
  /// includes features in the top dimension.
  void computePersistence(int homologyCoeffField = 11,
                          double minPersistence = 0.0,
                          bool persistenceDimMax = false) const;
  /// Convenience: compute and return the full diagram in one call.
  std::vector<PersistenceInterval> persistence(int homologyCoeffField = 11,
                                               double minPersistence = 0.0,
                                               bool persistenceDimMax = false) const;
  /// Full diagram after `computePersistence`.
  std::vector<PersistenceInterval> persistenceDiagram() const;
  std::vector<int> bettiNumbers() const;
  std::vector<int> persistentBettiNumbers(double from, double to) const;
  std::vector<BirthDeath> persistenceIntervalsInDimension(int dimension) const;
  std::vector<PersistencePair> persistencePairs() const;

  // Implementation handle (opaque to Swift).
  struct Impl;
  std::shared_ptr<Impl> impl;  // public so sibling builders can fill it directly
};

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_COMMON_HPP_
