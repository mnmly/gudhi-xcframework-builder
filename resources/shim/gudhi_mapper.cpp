// gudhi_mapper.cpp — implementation of the Swift-facing Mapper facade.
//
// All GUDHI (and Boost) usage is confined to this translation unit. It is
// self-contained and syntax-checkable with only GUDHI + Boost headers on the
// include path (no CGAL / GMP / Eigen / TBB):
//
//   clang++ -std=c++17 -fsyntax-only \
//       -I <gudhi-staged-include> -I $(brew --prefix boost)/include \
//       resources/shim/gudhi_mapper.cpp
//
// GUDHI's GIC.h only needs CGAL/Hera for compute_PD's bottleneck distance,
// which is guarded to throw at *runtime* (not compile time) when absent, so the
// header compiles cleanly without them.

#include "gudhi_mapper.hpp"
#include "internal/guard.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <unordered_set>
#include <vector>

#include <gudhi/GIC.h>
#include <gudhi/Simplex_tree.h>
#include <gudhi/distance_functions.h>

namespace gudhi_swift {
namespace {

// Concrete instantiation of GUDHI's templated Cover_complex, mirroring the
// Python binding's Nerve_gic_interface (src/python/gudhi/_nerve_gic.cc). Adding
// Euclidean-distance convenience wrappers and a graph extractor here lets us
// reach the (protected) create_complex from the same class hierarchy.
class Mapper_impl : public Gudhi::cover_complex::Cover_complex<std::vector<double>> {
 public:
  double set_graph_from_automatic_euclidean_rips(int N) {
    return set_graph_from_automatic_rips(Gudhi::Euclidean_distance(), N);
  }
  void set_graph_from_euclidean_rips(double threshold) {
    set_graph_from_rips(threshold, Gudhi::Euclidean_distance());
  }

  // Materialize the nerve's 1-skeleton into a flat MapperGraph. We use the
  // Nerve of the cover (type "Nerve"): GUDHI sets `simplices = cover`, so every
  // cover element appears as a vertex (isolated islands included) and edges
  // appear wherever two elements share a point (overlapping intervals).
  MapperGraph extract_graph() {
    this->set_type("Nerve");
    this->find_simplices();

    Gudhi::Simplex_tree<> st;
    this->create_complex(st);

    MapperGraph graph;
    for (auto sh : st.complex_simplex_range()) {
      const int dim = st.dimension(sh);
      if (dim == 0) {
        auto vr = st.simplex_vertex_range(sh);
        const int id = *vr.begin();
        MapperNode node;
        node.id = id;
        const std::vector<int>& pop = this->subpopulation(id);
        node.point_indices.assign(pop.begin(), pop.end());
        node.size = static_cast<int>(pop.size());
        node.color = this->subcolor(id);
        graph.nodes.push_back(std::move(node));
      } else if (dim == 1) {
        auto vr = st.simplex_vertex_range(sh);
        auto it = vr.begin();
        const int a = *it;
        ++it;
        const int b = *it;
        graph.edges.push_back(MapperEdge{a, b});
      }
    }
    return graph;
  }
};

// Drop nodes with <= mask points and any edge touching a removed node.
MapperGraph apply_mask(MapperGraph graph, int mask) {
  if (mask <= 0) return graph;
  std::unordered_set<int> kept;
  std::vector<MapperNode> nodes;
  nodes.reserve(graph.nodes.size());
  for (auto& n : graph.nodes) {
    if (n.size > mask) {
      kept.insert(n.id);
      nodes.push_back(std::move(n));
    }
  }
  std::vector<MapperEdge> edges;
  edges.reserve(graph.edges.size());
  for (const auto& e : graph.edges) {
    if (kept.count(e.source) && kept.count(e.target)) edges.push_back(e);
  }
  graph.nodes = std::move(nodes);
  graph.edges = std::move(edges);
  return graph;
}

// Lens + resolution/gain + functional pullback cover.
void configure_cover(Mapper_impl& impl, const MapperOptions& opt,
                     const std::vector<double>& lens, bool have_lens) {
  if (have_lens) {
    impl.set_function_from_range(lens);
  } else {
    impl.set_function_from_coordinate(opt.lensCoordinate);
  }
  impl.set_resolution_with_interval_number(opt.resolution);
  impl.set_gain(opt.gain);
  impl.set_cover_from_function();
}

std::vector<std::vector<double>> to_matrix(const double* data, int rows, int cols) {
  std::vector<std::vector<double>> m;
  m.reserve(rows);
  for (int i = 0; i < rows; ++i) m.emplace_back(data + i * cols, data + (i + 1) * cols);
  return m;
}

std::vector<double> to_vector(const double* data, int n) {
  return data ? std::vector<double>(data, data + n) : std::vector<double>();
}

}  // namespace

MapperGraph computeMapper(const double* points, int rows, int cols,
                          const double* lens, const double* color,
                          const MapperOptions& opt) {
 return detail::guard([&]() -> MapperGraph {
  MapperGraph empty;
  if (!points || rows <= 0 || cols <= 0) return empty;

  const std::vector<double> lensVec = to_vector(lens, rows);
  const std::vector<double> colorVec = to_vector(color, rows);

  Mapper_impl impl;
  impl.set_verbose(opt.verbose);
  impl.set_point_cloud_from_range(to_matrix(points, rows, cols));

  // Color (mean per node). Coordinates exist, so default to a coordinate.
  if (!colorVec.empty()) {
    impl.set_color_from_range(colorVec);
  } else {
    impl.set_color_from_coordinate(opt.colorCoordinate);
  }

  // Neighbourhood graph.
  if (opt.ripsThreshold < 0.0) {
    impl.set_graph_from_automatic_euclidean_rips(opt.automaticRipsN);
  } else {
    impl.set_graph_from_euclidean_rips(opt.ripsThreshold);
  }

  configure_cover(impl, opt, lensVec, /*have_lens=*/lens != nullptr);
  return apply_mask(impl.extract_graph(), opt.mask);
 });
}

MapperGraph computeMapperFromDistanceMatrix(const double* distances, int n,
                                            const double* lens, const double* color,
                                            const MapperOptions& opt) {
 return detail::guard([&]() -> MapperGraph {
  MapperGraph empty;
  // Without coordinates we cannot derive a lens or auto-tune the graph radius.
  if (!distances || n <= 0 || !lens || opt.ripsThreshold < 0.0) return empty;

  const std::vector<double> lensVec = to_vector(lens, n);
  const std::vector<double> colorVec = to_vector(color, n);

  Mapper_impl impl;
  impl.set_verbose(opt.verbose);
  impl.set_distances_from_range(to_matrix(distances, n, n));  // populates
      // `distances`, so set_graph_from_rips reuses it (the Euclidean functor is
      // never actually invoked).

  // No coordinates => color must come from a range; default to the lens values.
  impl.set_color_from_range(colorVec.empty() ? lensVec : colorVec);

  impl.set_graph_from_euclidean_rips(opt.ripsThreshold);

  configure_cover(impl, opt, lensVec, /*have_lens=*/true);
  return apply_mask(impl.extract_graph(), opt.mask);
 });
}

MapperGraph computeMapperND(const double* points, int rows, int cols,
                            const double* lens, int lensDim,
                            const double* color, const MapperOptions& opt) {
 return detail::guard([&]() -> MapperGraph {
  MapperGraph empty;
  if (!points || rows <= 0 || cols <= 0 || !lens || lensDim <= 0) return empty;

  const std::vector<double> colorVec = to_vector(color, rows);

  Mapper_impl impl;
  impl.set_verbose(opt.verbose);
  impl.set_point_cloud_from_range(to_matrix(points, rows, cols));

  if (!colorVec.empty()) {
    impl.set_color_from_range(colorVec);
  } else {
    impl.set_color_from_coordinate(opt.colorCoordinate);
  }

  // Neighbourhood graph (drives both clustering and the nerve's edges). The
  // automatic path returns the radius it chose, which we reuse for clustering.
  double thr;
  if (opt.ripsThreshold < 0.0) {
    thr = impl.set_graph_from_automatic_euclidean_rips(opt.automaticRipsN);
  } else {
    thr = opt.ripsThreshold;
    impl.set_graph_from_euclidean_rips(thr);
  }

  const int res = std::max(1, opt.resolution);
  const double gain = opt.gain;

  // Per-dimension overlapping intervals (GUDHI's interval scheme, generalised to
  // each lens axis): first/last cap to the data min/max, interiors overlap by α.
  std::vector<std::vector<std::pair<double, double>>> intervals(lensDim);
  for (int d = 0; d < lensDim; ++d) {
    double mn = (std::numeric_limits<double>::max)();
    double mx = std::numeric_limits<double>::lowest();
    for (int i = 0; i < rows; ++i) {
      const double v = lens[(std::size_t)i * lensDim + d];
      mn = (std::min)(mn, v);
      mx = (std::max)(mx, v);
    }
    auto& iv = intervals[d];
    if (res == 1 || mx <= mn) { iv.emplace_back(mn, mx); continue; }
    const double incr = (mx - mn) / res;
    const double alpha = (incr * gain) / (2 - 2 * gain);
    iv.emplace_back(mn, mn + incr + alpha);
    for (int i = 1; i < res - 1; ++i) iv.emplace_back(mn + i * incr - alpha, mn + (i + 1) * incr + alpha);
    iv.emplace_back(mn + (res - 1) * incr - alpha, mx);
  }

  // Mixed-radix multipliers so a per-axis interval tuple maps to one cell id.
  std::vector<long> radix(lensDim, 1);
  for (int d = 1; d < lensDim; ++d) radix[d] = radix[d - 1] * static_cast<long>(intervals[d - 1].size());

  // Group points by cell id (a point may fall in several cells via overlap).
  std::map<long, std::vector<int>> cellPoints;
  for (int i = 0; i < rows; ++i) {
    std::vector<long> ids = {0};
    bool ok = true;
    for (int d = 0; d < lensDim && ok; ++d) {
      const double v = lens[(std::size_t)i * lensDim + d];
      std::vector<int> mem;
      for (int k = 0; k < static_cast<int>(intervals[d].size()); ++k)
        if (v >= intervals[d][k].first && v <= intervals[d][k].second) mem.push_back(k);
      if (mem.empty()) { ok = false; break; }
      std::vector<long> next;
      next.reserve(ids.size() * mem.size());
      for (long base : ids) for (int k : mem) next.push_back(base + radix[d] * k);
      ids.swap(next);
    }
    if (!ok) continue;
    for (long c : ids) cellPoints[c].push_back(i);
  }

  // Squared-distance Rips test, reused for per-cell connected components.
  const double thr2 = thr * thr;
  auto dist2 = [&](int a, int b) {
    const double* pa = &points[(std::size_t)a * cols];
    const double* pb = &points[(std::size_t)b * cols];
    double s = 0;
    for (int c = 0; c < cols; ++c) { const double dd = pa[c] - pb[c]; s += dd * dd; }
    return s;
  };

  // Refine each cell into connected components → one cover element per component.
  // Contiguous ids (0..K-1) are required by set_cover_from_range's colour loop.
  std::vector<std::vector<int>> assignments(rows);
  int nextId = 0;
  for (auto& kv : cellPoints) {
    std::vector<int>& mem = kv.second;
    const int m = static_cast<int>(mem.size());
    std::vector<int> parent(m);
    for (int a = 0; a < m; ++a) parent[a] = a;
    std::function<int(int)> root = [&](int x) {
      while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
      return x;
    };
    for (int a = 0; a < m; ++a)
      for (int b = a + 1; b < m; ++b)
        if (dist2(mem[a], mem[b]) <= thr2) {
          const int ra = root(a), rb = root(b);
          if (ra != rb) parent[ra] = rb;
        }
    std::map<int, int> rootToId;
    for (int a = 0; a < m; ++a) {
      const int r = root(a);
      auto it = rootToId.find(r);
      const int gid = (it == rootToId.end()) ? (rootToId[r] = nextId++) : it->second;
      assignments[mem[a]].push_back(gid);
    }
  }

  if (nextId == 0) return empty;
  impl.set_cover_from_range(assignments);
  return apply_mask(impl.extract_graph(), opt.mask);
 });
}

// These are stamped by build.sh from the resolved upstream checkout. The
// fallbacks keep the facade compilable on its own (e.g. the fsyntax-only check).
#ifndef GUDHI_SWIFT_FACADE_VERSION
#define GUDHI_SWIFT_FACADE_VERSION "dev"
#endif
#ifndef GUDHI_SWIFT_GUDHI_VERSION
#define GUDHI_SWIFT_GUDHI_VERSION "unknown"
#endif
#ifndef GUDHI_SWIFT_GUDHI_DESCRIBE
#define GUDHI_SWIFT_GUDHI_DESCRIBE "unknown"
#endif

std::string version() {
  return "SwiftGUDHI facade " GUDHI_SWIFT_FACADE_VERSION
         " (GUDHI " GUDHI_SWIFT_GUDHI_VERSION ", " GUDHI_SWIFT_GUDHI_DESCRIBE ")";
}

}  // namespace gudhi_swift
