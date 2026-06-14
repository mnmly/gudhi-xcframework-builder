// common.cpp — SimplexTree + persistence implementation (mirrors GUDHI's
// Simplex_tree_interface / Persistent_cohomology_interface, de-templated).

#include "common.hpp"
#include "internal/gudhi_detail.hpp"
#include "internal/guard.hpp"

#include <algorithm>
#include <limits>
#include <tuple>

#include <gudhi/Flag_complex_edge_collapser.h>

namespace gudhi_swift {

using detail::ST;

namespace {

FilteredSimplex to_filtered(const ST& st, ST::Simplex_handle sh) {
  FilteredSimplex fs;
  for (auto v : st.simplex_vertex_range(sh)) fs.vertices.push_back(v);
  std::sort(fs.vertices.begin(), fs.vertices.end());
  fs.filtration = st.filtration(sh);
  return fs;
}

}  // namespace

SimplexTree::SimplexTree() : impl(std::make_shared<Impl>()) {}

// ── construction / mutation ──────────────────────────────────────────────────

bool SimplexTree::insert(const std::vector<int>& simplex, double filtration) const {
  auto result = impl->st.insert_simplex_and_subfaces(simplex, filtration);
  return result.second;
}

void SimplexTree::insertSimplices(const std::vector<FilteredSimplex>& simplices) const {
  for (const auto& s : simplices) impl->st.insert_simplex_and_subfaces(s.vertices, s.filtration);
}

void SimplexTree::assignFiltration(const std::vector<int>& simplex, double filtration) const {
  auto sh = impl->st.find(simplex);
  if (sh != impl->st.null_simplex()) impl->st.assign_filtration(sh, filtration);
}

bool SimplexTree::makeFiltrationNonDecreasing() const {
  return impl->st.make_filtration_non_decreasing();
}

bool SimplexTree::pruneAboveFiltration(double filtration) const {
  return impl->st.prune_above_filtration(filtration);
}

bool SimplexTree::pruneAboveDimension(int dimension) const {
  return impl->st.prune_above_dimension(dimension);
}

void SimplexTree::expansion(int maxDimension) const {
  if (maxDimension > 1) impl->st.expansion(maxDimension);
}

void SimplexTree::collapseEdges(int nbIterations) const {
  detail::guard([&]() -> int {
  using FilteredEdge = std::tuple<int, int, double>;
  auto& st = impl->st;
  std::vector<FilteredEdge> edges;
  for (auto sh : st.skeleton_simplex_range(1)) {
    if (st.dimension(sh) != 1) continue;
    auto rg = st.simplex_vertex_range(sh);
    auto it = rg.begin();
    const int v = *it;
    const int w = *++it;
    edges.emplace_back(v, w, st.filtration(sh));
  }
  st.prune_above_dimension(0);  // keep only vertices
  for (int i = 0; i < nbIterations; ++i)
    edges = Gudhi::collapse::flag_complex_collapse_edges(std::move(edges));
  for (const auto& e : edges)
    st.insert_simplex_and_subfaces({std::get<0>(e), std::get<1>(e)}, std::get<2>(e));
  return 0;
  });
}

void SimplexTree::resetFiltration(double filtration, int minDimension) const {
  impl->st.reset_filtration(filtration, minDimension);
}

void SimplexTree::clear() const { impl->st.clear(); }

// ── queries ──────────────────────────────────────────────────────────────────

int SimplexTree::numVertices() const { return static_cast<int>(impl->st.num_vertices()); }
int SimplexTree::numSimplices() const { return static_cast<int>(impl->st.num_simplices()); }
int SimplexTree::dimension() const { return impl->st.dimension(); }

bool SimplexTree::find(const std::vector<int>& simplex) const {
  return impl->st.find(simplex) != impl->st.null_simplex();
}

double SimplexTree::filtration(const std::vector<int>& simplex) const {
  auto sh = impl->st.find(simplex);
  if (sh == impl->st.null_simplex()) return std::numeric_limits<double>::quiet_NaN();
  return impl->st.filtration(sh);
}

std::vector<FilteredSimplex> SimplexTree::getSimplices() const {
  std::vector<FilteredSimplex> out;
  for (auto sh : impl->st.complex_simplex_range()) out.push_back(to_filtered(impl->st, sh));
  return out;
}

std::vector<FilteredSimplex> SimplexTree::getSkeleton(int dimension) const {
  std::vector<FilteredSimplex> out;
  for (auto sh : impl->st.skeleton_simplex_range(dimension)) out.push_back(to_filtered(impl->st, sh));
  return out;
}

std::vector<FilteredSimplex> SimplexTree::getStar(const std::vector<int>& simplex) const {
  std::vector<FilteredSimplex> out;
  auto sh = impl->st.find(simplex);
  if (sh == impl->st.null_simplex()) return out;
  for (auto cf : impl->st.star_simplex_range(sh)) out.push_back(to_filtered(impl->st, cf));
  return out;
}

std::vector<FilteredSimplex> SimplexTree::getCofaces(const std::vector<int>& simplex,
                                                     int codimension) const {
  std::vector<FilteredSimplex> out;
  auto sh = impl->st.find(simplex);
  if (sh == impl->st.null_simplex()) return out;
  for (auto cf : impl->st.cofaces_simplex_range(sh, codimension)) out.push_back(to_filtered(impl->st, cf));
  return out;
}

std::vector<FilteredSimplex> SimplexTree::getBoundaries(const std::vector<int>& simplex) const {
  std::vector<FilteredSimplex> out;
  auto sh = impl->st.find(simplex);
  if (sh == impl->st.null_simplex()) return out;
  for (auto b : impl->st.boundary_simplex_range(sh)) out.push_back(to_filtered(impl->st, b));
  return out;
}

// ── persistence ──────────────────────────────────────────────────────────────

void SimplexTree::computePersistence(int homologyCoeffField, double minPersistence,
                                     bool persistenceDimMax) const {
  detail::guard([&]() -> int {
    impl->pcoh = std::make_unique<detail::Pcoh>(impl->st, persistenceDimMax);
    impl->pcoh->init_coefficients(homologyCoeffField);
    impl->pcoh->compute_persistent_cohomology(minPersistence);
    return 0;
  });
}

std::vector<PersistenceInterval> SimplexTree::persistenceDiagram() const {
  std::vector<PersistenceInterval> out;
  if (!impl->pcoh) return out;
  const auto& pairs = impl->pcoh->get_persistent_pairs();
  out.reserve(pairs.size());
  for (const auto& pair : pairs) {
    PersistenceInterval pi;
    pi.dimension = impl->st.dimension(std::get<0>(pair));
    pi.birth = impl->st.filtration(std::get<0>(pair));
    pi.death = impl->st.filtration(std::get<1>(pair));  // filtration(null) == +inf
    out.push_back(pi);
  }
  std::sort(out.begin(), out.end(), [](const PersistenceInterval& a, const PersistenceInterval& b) {
    if (a.dimension != b.dimension) return a.dimension > b.dimension;
    return (a.death - a.birth) > (b.death - b.birth);
  });
  return out;
}

std::vector<PersistenceInterval> SimplexTree::persistence(int homologyCoeffField,
                                                          double minPersistence,
                                                          bool persistenceDimMax) const {
  computePersistence(homologyCoeffField, minPersistence, persistenceDimMax);
  return persistenceDiagram();
}

std::vector<int> SimplexTree::bettiNumbers() const {
  if (!impl->pcoh) return {};
  return impl->pcoh->betti_numbers();
}

std::vector<int> SimplexTree::persistentBettiNumbers(double from, double to) const {
  if (!impl->pcoh) return {};
  return impl->pcoh->persistent_betti_numbers(from, to);
}

std::vector<BirthDeath> SimplexTree::persistenceIntervalsInDimension(int dimension) const {
  std::vector<BirthDeath> out;
  if (!impl->pcoh) return out;
  for (const auto& pair : impl->pcoh->get_persistent_pairs()) {
    if (impl->st.dimension(std::get<0>(pair)) != dimension) continue;
    out.push_back(BirthDeath{impl->st.filtration(std::get<0>(pair)),
                             impl->st.filtration(std::get<1>(pair))});
  }
  std::sort(out.begin(), out.end(),
            [](const BirthDeath& a, const BirthDeath& b) { return a.birth < b.birth; });
  return out;
}

std::vector<PersistencePair> SimplexTree::persistencePairs() const {
  std::vector<PersistencePair> out;
  if (!impl->pcoh) return out;
  const auto& pairs = impl->pcoh->get_persistent_pairs();
  out.reserve(pairs.size());
  for (const auto& pair : pairs) {
    PersistencePair pp;
    if (std::get<0>(pair) != impl->st.null_simplex())
      for (auto v : impl->st.simplex_vertex_range(std::get<0>(pair))) pp.birth.push_back(v);
    if (std::get<1>(pair) != impl->st.null_simplex())
      for (auto v : impl->st.simplex_vertex_range(std::get<1>(pair))) pp.death.push_back(v);
    out.push_back(std::move(pp));
  }
  return out;
}

}  // namespace gudhi_swift
