// gudhi_mapper.hpp — de-templated, Swift-facing facade over GUDHI's
// Nerve / Graph-Induced-Complex (Mapper) machinery.
//
// This header is the ONLY thing shipped in the xcframework's Headers/ dir and
// the entry point named by module.modulemap. It is consumed from Swift with
// C++ interoperability enabled (`-cxx-interoperability-mode`).
//
// Interop design:
//   * INPUTS are passed as raw pointers + sizes and a scalar-only options
//     struct. Both are trivial to produce from Swift (`withUnsafeBufferPointer`
//     + a default-constructed struct), avoiding the awkwardness of building
//     nested std::vector from Swift.
//   * OUTPUT is a std::vector-based graph returned by value. Reading std::vector
//     from Swift via CxxStdlib is well supported (it conforms to Collection).
//   * Public signatures use only POD / std types, so this header needs no
//     GUDHI/Boost include and stays syntax-checkable on its own. All GUDHI usage
//     is hidden in gudhi_mapper.cpp.
//
// Mapper, here, is the *nerve of a functional pullback cover*: cover the image
// of a 1-D lens function with overlapping intervals (resolution + gain), refine
// each interval's preimage into connected components of a neighbourhood (Rips)
// graph — those components are the nodes — and connect two nodes whenever they
// share a point (i.e. they live in overlapping intervals). This is classic
// Mapper: dense clusters become nodes, overlap becomes edges, and isolated
// "islands" survive as lone nodes.

#ifndef GUDHI_SWIFT_MAPPER_HPP_
#define GUDHI_SWIFT_MAPPER_HPP_

#include <string>
#include <vector>

namespace gudhi_swift {

/// One node of the Mapper graph: a cluster of input points.
struct MapperNode {
  int id = 0;                       ///< GUDHI cover-element id (stable, may be sparse)
  int size = 0;                     ///< number of points in this node
  double color = 0.0;               ///< mean color value over the node's points
  std::vector<int> point_indices;   ///< indices (into the input) of member points
};

/// One undirected edge: the two endpoint node ids share at least one point.
struct MapperEdge {
  int source = 0;
  int target = 0;
};

/// The computed Mapper / Nerve / GIC 1-skeleton.
struct MapperGraph {
  std::vector<MapperNode> nodes;
  std::vector<MapperEdge> edges;
};

/// Tuning options. Scalars only, so Swift can default-construct one and set
/// properties directly. Defaults give a usable Mapper out of the box.
struct MapperOptions {
  int resolution = 10;           ///< number of overlapping lens intervals
  double gain = 0.3;             ///< interval overlap fraction in [0, 0.5)
  int lensCoordinate = 0;        ///< coordinate used as the lens when no lens ptr is given
  double ripsThreshold = -1.0;   ///< neighbourhood-graph radius; < 0 => auto-tuned
  int automaticRipsN = 100;      ///< subsampling iterations for the auto threshold
  int colorCoordinate = 0;       ///< coordinate used for color when no color ptr is given
  int mask = 0;                  ///< drop nodes with <= mask points (0 keeps all)
  bool verbose = false;          ///< log progress to stderr
};

/// Build a Mapper graph from a point cloud / embedding matrix.
///
/// @param points  row-major matrix, length rows*cols (point i = points[i*cols .. ]).
/// @param rows    number of points.
/// @param cols    dimension of each point.
/// @param lens    optional length-`rows` lens (one scalar per point); null => use
///                `opt.lensCoordinate`.
/// @param color   optional length-`rows` color values; null => use `opt.colorCoordinate`.
MapperGraph computeMapper(const double* points, int rows, int cols,
                          const double* lens, const double* color,
                          const MapperOptions& opt);

/// Build a Mapper graph from a precomputed pairwise distance matrix.
///
/// @param distances  row-major n*n matrix.
/// @param n          number of points.
/// @param lens       REQUIRED length-`n` lens (no coordinates exist to derive one).
/// @param color      optional length-`n` color; null => reuse the lens values.
/// `opt.ripsThreshold` MUST be >= 0 (auto-tuning needs coordinates). Returns an
/// empty graph if these preconditions are not met.
MapperGraph computeMapperFromDistanceMatrix(const double* distances, int n,
                                            const double* lens, const double* color,
                                            const MapperOptions& opt);

/// Build a Mapper graph from a point cloud using an **N-dimensional lens**.
///
/// GUDHI's functional cover is 1-D only, so this builds the N-D hypercube cover
/// itself: each lens coordinate is split into `opt.resolution` overlapping
/// intervals (gain overlap); their Cartesian product is the cover, and each
/// cell is refined into connected components of the Rips neighbourhood graph
/// (at the same threshold GUDHI would use) — those components are the nodes.
/// Edges come from GUDHI's nerve over the supplied cover. With `lensDim == 1`
/// this matches `computeMapper`; `lensDim == 2/3/4` yields a grid/branching atlas.
///
/// @param points   row-major matrix, length rows*cols.
/// @param rows     number of points.
/// @param cols     dimension of each point.
/// @param lens     REQUIRED row-major `rows*lensDim` lens (point i's lens =
///                 lens[i*lensDim .. i*lensDim+lensDim-1]).
/// @param lensDim  number of lens dimensions (>= 1).
/// @param color    optional length-`rows` color; null => use `opt.colorCoordinate`.
MapperGraph computeMapperND(const double* points, int rows, int cols,
                            const double* lens, int lensDim,
                            const double* color, const MapperOptions& opt);

/// Library version string.
std::string version();

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_MAPPER_HPP_
