// tangential.cpp — Tangential complex over CGAL.

#include "tangential.hpp"
#include "internal/gudhi_detail.hpp"
#include "internal/guard.hpp"

#include <vector>

#include <CGAL/Epick_d.h>
#include <gudhi/Tangential_complex.h>

namespace gudhi_swift {

SimplexTree tangentialComplex(const double* points, int rows, int cols, int intrinsicDimension) {
  return detail::guard([&]() -> SimplexTree {
    SimplexTree tree;
    if (!points || rows <= 0 || cols <= 0) return tree;

    using Kernel = CGAL::Epick_d<CGAL::Dynamic_dimension_tag>;
    using TC = Gudhi::tangential_complex::Tangential_complex<
        Kernel, CGAL::Dynamic_dimension_tag, CGAL::Parallel_tag>;

    std::vector<std::vector<double>> pts;
    pts.reserve(rows);
    for (int i = 0; i < rows; ++i) pts.emplace_back(points + i * cols, points + (i + 1) * cols);

    TC tc(pts, intrinsicDimension, Kernel());
    tc.compute_tangential_complex();
    tc.create_complex(tree.impl->st);
    return tree;
  });
}

}  // namespace gudhi_swift
