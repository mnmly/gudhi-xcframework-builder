// alpha.cpp — Alpha complex over CGAL (Epick_d / Epeck_d).

#include "alpha.hpp"
#include "internal/gudhi_detail.hpp"
#include "internal/guard.hpp"

#include <vector>

#include <CGAL/Epeck_d.h>
#include <CGAL/Epick_d.h>
#include <gudhi/Alpha_complex.h>

namespace gudhi_swift {
namespace {

std::vector<std::vector<double>> to_matrix(const double* data, int rows, int cols) {
  std::vector<std::vector<double>> m;
  m.reserve(rows);
  for (int i = 0; i < rows; ++i) m.emplace_back(data + i * cols, data + (i + 1) * cols);
  return m;
}

template <typename Kernel>
SimplexTree build_alpha(const std::vector<std::vector<double>>& pts,
                        double maxAlphaSquare, bool exact, bool outputSquared) {
  using Point = typename Kernel::Point_d;
  std::vector<Point> cgalPoints;
  cgalPoints.reserve(pts.size());
  for (const auto& v : pts) cgalPoints.emplace_back(static_cast<int>(v.size()), v.begin(), v.end());

  Gudhi::alpha_complex::Alpha_complex<Kernel> ac(cgalPoints);
  SimplexTree tree;
  if (outputSquared)
    ac.template create_complex<true>(tree.impl->st, maxAlphaSquare, exact, false);
  else
    ac.template create_complex<false>(tree.impl->st, maxAlphaSquare, exact, false);
  return tree;
}

}  // namespace

SimplexTree alphaComplex(const double* points, int rows, int cols,
                         double maxAlphaSquare, AlphaPrecision precision,
                         bool outputSquared) {
  return detail::guard([&]() -> SimplexTree {
    if (!points || rows <= 0 || cols <= 0) return SimplexTree();
    const auto pts = to_matrix(points, rows, cols);

    using Epick = CGAL::Epick_d<CGAL::Dynamic_dimension_tag>;
    using Epeck = CGAL::Epeck_d<CGAL::Dynamic_dimension_tag>;

    if (precision == AlphaPrecision::fast)
      return build_alpha<Epick>(pts, maxAlphaSquare, /*exact=*/false, outputSquared);
    const bool exact = (precision == AlphaPrecision::exact);
    return build_alpha<Epeck>(pts, maxAlphaSquare, exact, outputSquared);
  });
}

}  // namespace gudhi_swift
