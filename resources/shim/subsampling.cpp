// subsampling.cpp — GUDHI point-cloud subsampling.

#include "subsampling.hpp"
#include "internal/guard.hpp"

#include <cstddef>
#include <iterator>
#include <vector>

#include <CGAL/Epick_d.h>
#include <gudhi/choose_n_farthest_points.h>
#include <gudhi/pick_n_random_points.h>
#include <gudhi/sparsify_point_set.h>
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

DoubleMatrix chooseNFarthestPoints(const double* points, int rows, int cols,
                                   int nbPoints, int startingPoint, bool metric) {
  return detail::guard([&]() -> DoubleMatrix {
    DoubleMatrix landmarks;
    if (!points || rows <= 0 || cols <= 0) return landmarks;
    const auto pts = to_matrix(points, rows, cols);
    const std::size_t start =
        startingPoint < 0 ? Gudhi::subsampling::random_starting_point
                          : static_cast<std::size_t>(startingPoint);
    if (metric)
      Gudhi::subsampling::choose_n_farthest_points_metric(
          Gudhi::Euclidean_distance(), pts, nbPoints, start, std::back_inserter(landmarks));
    else
      Gudhi::subsampling::choose_n_farthest_points(
          Gudhi::Euclidean_distance(), pts, nbPoints, start, std::back_inserter(landmarks));
    return landmarks;
  });
}

DoubleMatrix pickNRandomPoints(const double* points, int rows, int cols, int nbPoints) {
  return detail::guard([&]() -> DoubleMatrix {
    DoubleMatrix landmarks;
    if (!points || rows <= 0 || cols <= 0) return landmarks;
    const auto pts = to_matrix(points, rows, cols);
    Gudhi::subsampling::pick_n_random_points(pts, nbPoints, std::back_inserter(landmarks));
    return landmarks;
  });
}

DoubleMatrix sparsifyPointSet(const double* points, int rows, int cols, double minSquaredDist) {
  return detail::guard([&]() -> DoubleMatrix {
    DoubleMatrix landmarks;
    if (!points || rows <= 0 || cols <= 0) return landmarks;
    using Kernel = CGAL::Epick_d<CGAL::Dynamic_dimension_tag>;
    using Point_d = Kernel::Point_d;

    std::vector<Point_d> input, output;
    input.reserve(rows);
    for (int i = 0; i < rows; ++i) {
      const double* row = points + i * cols;
      input.emplace_back(cols, row, row + cols);
    }
    Kernel k;
    Gudhi::subsampling::sparsify_point_set(k, input, minSquaredDist, std::back_inserter(output));
    for (const auto& p : output) landmarks.emplace_back(p.cartesian_begin(), p.cartesian_end());
    return landmarks;
  });
}

}  // namespace gudhi_swift
