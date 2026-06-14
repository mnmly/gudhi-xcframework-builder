// cubical.cpp — Bitmap cubical complex + persistence.

#include "cubical.hpp"
#include "internal/guard.hpp"

#include <algorithm>
#include <tuple>
#include <vector>

#include <gudhi/Bitmap_cubical_complex.h>
#include <gudhi/Bitmap_cubical_complex_base.h>
#include <gudhi/Persistent_cohomology.h>

namespace gudhi_swift {
namespace {

using Cubical = Gudhi::cubical_complex::Bitmap_cubical_complex<
    Gudhi::cubical_complex::Bitmap_cubical_complex_base<double>>;
using PcohCub = Gudhi::persistent_cohomology::Persistent_cohomology<
    Cubical, Gudhi::persistent_cohomology::Field_Zp>;

}  // namespace

struct CubicalComplex::Impl {
  std::unique_ptr<Cubical> cc;
  std::unique_ptr<PcohCub> pcoh;

  void ensurePersistence(int field, double minPersistence) {
    pcoh = std::make_unique<PcohCub>(*cc, true);
    pcoh->init_coefficients(field);
    pcoh->compute_persistent_cohomology(minPersistence);
  }
};

CubicalComplex::CubicalComplex() : impl(std::make_shared<Impl>()) {}

int CubicalComplex::numSimplices() const {
  return impl->cc ? static_cast<int>(impl->cc->num_simplices()) : 0;
}

int CubicalComplex::dimension() const {
  return impl->cc ? static_cast<int>(impl->cc->dimension()) : -1;
}

std::vector<PersistenceInterval> CubicalComplex::persistence(int homologyCoeffField,
                                                             double minPersistence) const {
 return detail::guard([&]() -> std::vector<PersistenceInterval> {
  std::vector<PersistenceInterval> out;
  if (!impl->cc) return out;
  impl->ensurePersistence(homologyCoeffField, minPersistence);
  for (const auto& pair : impl->pcoh->get_persistent_pairs()) {
    PersistenceInterval pi;
    pi.dimension = impl->cc->dimension(std::get<0>(pair));
    pi.birth = impl->cc->filtration(std::get<0>(pair));
    pi.death = impl->cc->filtration(std::get<1>(pair));  // filtration(null) == +inf
    out.push_back(pi);
  }
  std::sort(out.begin(), out.end(), [](const PersistenceInterval& a, const PersistenceInterval& b) {
    if (a.dimension != b.dimension) return a.dimension > b.dimension;
    return (a.death - a.birth) > (b.death - b.birth);
  });
  return out;
 });
}

std::vector<int> CubicalComplex::bettiNumbers() const {
  if (!impl->pcoh) return {};
  return impl->pcoh->betti_numbers();
}

std::vector<BirthDeath> CubicalComplex::persistenceIntervalsInDimension(int dimension) const {
  std::vector<BirthDeath> out;
  if (!impl->pcoh) return out;
  for (const auto& pair : impl->pcoh->get_persistent_pairs()) {
    if (impl->cc->dimension(std::get<0>(pair)) != dimension) continue;
    out.push_back(BirthDeath{impl->cc->filtration(std::get<0>(pair)),
                             impl->cc->filtration(std::get<1>(pair))});
  }
  std::sort(out.begin(), out.end(),
            [](const BirthDeath& a, const BirthDeath& b) { return a.birth < b.birth; });
  return out;
}

CubicalComplex cubicalComplex(const int* dimensions, int ndims,
                              const double* cells, int ncells,
                              bool inputTopCells) {
  return detail::guard([&]() -> CubicalComplex {
    CubicalComplex complex;
    if (!dimensions || ndims <= 0 || !cells || ncells <= 0) return complex;
    std::vector<unsigned> dims(dimensions, dimensions + ndims);
    std::vector<double> values(cells, cells + ncells);
    complex.impl->cc = std::make_unique<Cubical>(dims, values, inputTopCells);
    return complex;
  });
}

}  // namespace gudhi_swift
