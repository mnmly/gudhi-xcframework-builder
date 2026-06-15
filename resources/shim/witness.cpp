// witness.cpp — table-based witness complex (combinatorial; no CGAL).

#include "witness.hpp"
#include "internal/gudhi_detail.hpp"
#include "internal/guard.hpp"

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include <gudhi/Witness_complex.h>

namespace gudhi_swift {
namespace {

std::size_t dim_limit(int limitDimension) {
  return limitDimension < 0 ? std::numeric_limits<std::size_t>::max()
                            : static_cast<std::size_t>(limitDimension);
}

}  // namespace

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
