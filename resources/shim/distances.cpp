// distances.cpp — bottleneck + Wasserstein via the bundled Hera (header-only).

#include "distances.hpp"

#include <limits>
#include <utility>
#include <vector>

#include <hera/bottleneck.h>
#include <hera/wasserstein.h>

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
}

namespace gudhi_swift {
namespace {

std::vector<std::pair<double, double>> to_diagram(const double* data, int n) {
  std::vector<std::pair<double, double>> dgm;
  if (!data || n <= 0) return dgm;
  dgm.reserve(n);
  for (int i = 0; i < n; ++i) dgm.emplace_back(data[2 * i], data[2 * i + 1]);
  return dgm;
}

}  // namespace

double bottleneckDistance(const double* diagram1, int n1,
                          const double* diagram2, int n2,
                          double delta) {
  try {
    auto d1 = to_diagram(diagram1, n1);
    auto d2 = to_diagram(diagram2, n2);
    if (delta == 0.0) return hera::bottleneckDistExact(d1, d2);
    return hera::bottleneckDistApprox(d1, d2, delta);
  } catch (...) {
    return kNaN;  // never let a C++ exception cross into Swift (would terminate)
  }
}

double wassersteinDistance(const double* diagram1, int n1,
                           const double* diagram2, int n2,
                           double order, double internalP, double delta) {
  try {
    auto d1 = to_diagram(diagram1, n1);
    auto d2 = to_diagram(diagram2, n2);
    hera::AuctionParams<double> params;
    params.wasserstein_power = order;
    params.delta = delta;
    params.internal_p = internalP < 0 ? std::numeric_limits<double>::infinity() : internalP;
    // Return a best-effort cost instead of throwing if the auction hits its
    // iteration cap — a thrown std::runtime_error would terminate the Swift app.
    params.tolerate_max_iter_exceeded = true;
    return hera::wasserstein_dist(d1, d2, params);
  } catch (...) {
    return kNaN;
  }
}

}  // namespace gudhi_swift
