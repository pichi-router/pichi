#ifndef PICHI_ACTOR_DETACHED_HPP
#define PICHI_ACTOR_DETACHED_HPP

#include <stdexcept>

namespace pichi::actor {

struct Detached {
  static void handler(std::exception_ptr);

  constexpr Detached() = default;

  template <typename... Args> void operator()(std::exception_ptr ep, Args&&...) { handler(ep); }
};

inline constexpr Detached detached;

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_DETACHED_HPP
