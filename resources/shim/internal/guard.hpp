// internal/guard.hpp — exception barrier at the Swift/C++ boundary.
//
// Swift cannot catch C++ exceptions: if one reaches a Swift call site the
// program terminates. GUDHI / CGAL / Hera can throw on degenerate or extreme
// input (e.g. Hera's auction on non-convergence). Every public facade entry
// point runs its body through guard(), which converts any C++ exception into a
// default-constructed result (empty SimplexTree / empty vector / 0.0) instead of
// letting it cross into Swift. NOT shipped to Swift (lives under internal/).

#ifndef GUDHI_SWIFT_INTERNAL_GUARD_HPP_
#define GUDHI_SWIFT_INTERNAL_GUARD_HPP_

#include <utility>

namespace gudhi_swift {
namespace detail {

template <class F>
inline auto guard(F&& f) -> decltype(f()) {
  using R = decltype(f());
  try {
    return std::forward<F>(f)();
  } catch (...) {
    return R{};
  }
}

}  // namespace detail
}  // namespace gudhi_swift

#endif  // GUDHI_SWIFT_INTERNAL_GUARD_HPP_
