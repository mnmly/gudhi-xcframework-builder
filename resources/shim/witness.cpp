// witness.cpp — Euclidean and table-based witness complexes (CGAL).

#include "witness.hpp"
#include "internal/gudhi_detail.hpp"
#include "internal/guard.hpp"

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include <CGAL/Epick_d.h>
#include <gudhi/Euclidean_witness_complex.h>
#include <gudhi/Witness_complex.h>

namespace gudhi_swift {
namespace {

using Kernel = CGAL::Epick_d<CGAL::Dynamic_dimension_tag>;
using Point_d = Kernel::Point_d;

std::size_t dim_limit(int limitDimension) {
  return limitDimension < 0 ? std::numeric_limits<std::size_t>::max()
                            : static_cast<std::size_t>(limitDimension);
}

}  // namespace

SimplexTree euclideanWitnessComplex(const double* landmarks, int numLandmarks, int landmarkDim,
                                    const double* witnesses, int numWitnesses, int witnessDim,
                                    double maxAlphaSquare, int limitDimension) {
  return detail::guard([&]() -> SimplexTree {
    SimplexTree tree;
    if (!landmarks || numLandmarks <= 0 || !witnesses || numWitnesses <= 0) return tree;

    std::vector<Point_d> lm;
    lm.reserve(numLandmarks);
    for (int i = 0; i < numLandmarks; ++i) {
      const double* row = landmarks + i * landmarkDim;
      lm.emplace_back(row, row + landmarkDim);
    }
    std::vector<std::vector<double>> wit;
    wit.reserve(numWitnesses);
    for (int i = 0; i < numWitnesses; ++i)
      wit.emplace_back(witnesses + i * witnessDim, witnesses + (i + 1) * witnessDim);

    Gudhi::witness_complex::Euclidean_witness_complex<Kernel> wc(lm, wit);
    wc.create_complex(tree.impl->st, maxAlphaSquare, dim_limit(limitDimension));
    return tree;
  });
}

SimplexTree witnessComplexFromTable(const int* landmarkIndices, const double* squaredDistances,
                                    int numWitnesses, int k,
                                    double maxAlphaSquare, int limitDimension) {
  return detail::guard([&]() -> SimplexTree {
    SimplexTree tree;
    if (!landmarkIndices || !squaredDistances || numWitnesses <= 0 || k <= 0) return tree;

    using Range = std::vector<std::pair<std::size_t, double>>;
    std::vector<Range> table;
    table.reserve(numWitnesses);
    for (int w = 0; w < numWitnesses; ++w) {
      Range r;
      r.reserve(k);
      for (int j = 0; j < k; ++j) {
        const int idx = w * k + j;
        r.emplace_back(static_cast<std::size_t>(landmarkIndices[idx]), squaredDistances[idx]);
      }
      table.push_back(std::move(r));
    }

    Gudhi::witness_complex::Witness_complex<std::vector<Range>> wc(table);
    wc.create_complex(tree.impl->st, maxAlphaSquare, dim_limit(limitDimension));
    return tree;
  });
}

}  // namespace gudhi_swift
