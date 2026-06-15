// subsampling.cpp — GUDHI point-cloud subsampling (no CGAL).

#include "subsampling.hpp"
#include "internal/guard.hpp"

#include <cstddef>
#include <iterator>
#include <vector>

#include <gudhi/choose_n_farthest_points.h>
#include <gudhi/pick_n_random_points.h>
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

}  // namespace gudhi_swift
