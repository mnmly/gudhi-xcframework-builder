// rips.cpp — Rips / Sparse-Rips builders over GUDHI.

#include "rips.hpp"
#include "internal/gudhi_detail.hpp"
#include "internal/guard.hpp"

#include <limits>
#include <vector>

#include <gudhi/Rips_complex.h>
#include <gudhi/Sparse_rips_complex.h>
#include <gudhi/distance_functions.h>

namespace gudhi_swift {
namespace {

std::vector<std::vector<double>> to_matrix(const double* data, int rows, int cols) {
  std::vector<std::vector<double>> m;
  m.reserve(rows);
  for (int i = 0; i < rows; ++i) m.emplace_back(data + i * cols, data + (i + 1) * cols);
  return m;
}

}  // namespace

SimplexTree ripsComplexFromPoints(const double* points, int rows, int cols,
                                  double maxEdgeLength, int maxDimension) {
  return detail::guard([&]() -> SimplexTree {
    SimplexTree tree;
    if (!points || rows <= 0 || cols <= 0) return tree;
    const auto pts = to_matrix(points, rows, cols);
    Gudhi::rips_complex::Rips_complex<double> rips(pts, maxEdgeLength, Gudhi::Euclidean_distance());
    rips.create_complex(tree.impl->st, maxDimension);
    return tree;
  });
}

SimplexTree ripsComplexFromDistanceMatrix(const double* distances, int n,
                                          double maxEdgeLength, int maxDimension) {
  return detail::guard([&]() -> SimplexTree {
    SimplexTree tree;
    if (!distances || n <= 0) return tree;
    const auto mat = to_matrix(distances, n, n);
    Gudhi::rips_complex::Rips_complex<double> rips(mat, maxEdgeLength);
    rips.create_complex(tree.impl->st, maxDimension);
    return tree;
  });
}

SimplexTree sparseRipsFromPoints(const double* points, int rows, int cols,
                                 double epsilon, double maxEdgeLength, int maxDimension) {
  return detail::guard([&]() -> SimplexTree {
    SimplexTree tree;
    if (!points || rows <= 0 || cols <= 0) return tree;
    const auto pts = to_matrix(points, rows, cols);
    Gudhi::rips_complex::Sparse_rips_complex<double> rips(
        pts, Gudhi::Euclidean_distance(), epsilon,
        -std::numeric_limits<double>::infinity(), maxEdgeLength);
    rips.create_complex(tree.impl->st, maxDimension);
    return tree;
  });
}

SimplexTree sparseRipsFromDistanceMatrix(const double* distances, int n,
                                         double epsilon, double maxEdgeLength, int maxDimension) {
  return detail::guard([&]() -> SimplexTree {
    SimplexTree tree;
    if (!distances || n <= 0) return tree;
    const auto mat = to_matrix(distances, n, n);
    Gudhi::rips_complex::Sparse_rips_complex<double> rips(
        mat, epsilon, -std::numeric_limits<double>::infinity(), maxEdgeLength);
    rips.create_complex(tree.impl->st, maxDimension);
    return tree;
  });
}

}  // namespace gudhi_swift
