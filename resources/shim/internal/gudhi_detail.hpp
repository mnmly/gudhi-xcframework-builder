// internal/gudhi_detail.hpp — INTERNAL to the facade; NOT shipped to Swift.
//
// Defines the concrete GUDHI types behind the public SimplexTree handle, so the
// per-module translation units (common.cpp, rips.cpp, alpha.cpp, ...) can share
// one Simplex_tree instantiation and fill it directly. build.sh ships only the
// top-level resources/shim/*.hpp; this file lives under internal/ and stays out
// of the xcframework Headers/.

#ifndef GUDHI_SWIFT_INTERNAL_DETAIL_HPP_
#define GUDHI_SWIFT_INTERNAL_DETAIL_HPP_

#include <cstdint>
#include <memory>

#include <gudhi/Simplex_tree.h>
#include <gudhi/Persistent_cohomology.h>

#include "../common.hpp"

namespace gudhi_swift {
namespace detail {

// Mirror of GUDHI's Simplex_tree_options_for_python (which we can't include —
// it pulls in nanobind). Same concrete types so persistence behaves identically.
struct Options_for_swift {
  typedef Gudhi::linear_indexing_tag Indexing_tag;
  typedef int Vertex_handle;
  typedef double Filtration_value;
  typedef std::uint32_t Simplex_key;
  static const bool store_key = true;
  static const bool store_filtration = true;
  static const bool contiguous_vertices = false;
  static const bool link_nodes_by_label = false;
  static const bool stable_simplex_handles = false;
};

using ST = Gudhi::Simplex_tree<Options_for_swift>;
using Pcoh = Gudhi::persistent_cohomology::Persistent_cohomology<ST, Gudhi::persistent_cohomology::Field_Zp>;

}  // namespace detail

// Concrete backing store of the public SimplexTree handle.
struct SimplexTree::Impl {
  detail::ST st;
  std::unique_ptr<detail::Pcoh> pcoh;  // recreated by computePersistence()
};

// Build a fresh SimplexTree handle wrapping a default-constructed Impl. Sibling
// builders use this, then fill `tree.impl->st`.
inline SimplexTree make_simplex_tree() {
  SimplexTree t;  // its ctor already allocates an Impl
  return t;
}

}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_INTERNAL_DETAIL_HPP_
