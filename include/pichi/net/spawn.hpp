#ifndef PICHI_NET_SPAWN_HPP
#define PICHI_NET_SPAWN_HPP

#include <boost/asio/spawn2.hpp>
#include <exception>
#include <type_traits>
#include <utility>

namespace pichi::net {

extern void logException(std::exception_ptr) noexcept;
extern void stubHandler(std::exception_ptr) noexcept;

template <typename T, typename Function, typename ExceptionHandler = decltype(stubHandler)>
void spawn(T&& t, Function&& f, ExceptionHandler&& eh = stubHandler)
{
  static_assert(std::is_invocable_v<std::decay_t<Function>, boost::asio::yield_context>);
  static_assert(std::is_nothrow_invocable_v<std::decay_t<ExceptionHandler>, std::exception_ptr>);
  boost::asio::spawn(
      std::forward<T>(t),
      [f = std::forward<Function>(f), eh = std::forward<ExceptionHandler>(eh)](auto yield) mutable {
        try {
          f(yield);
        }
        catch (...) {
          auto eptr = std::current_exception();
          eh(eptr);
          logException(eptr);
        }
      });
}

} // namespace pichi::net

#endif // PICHI_NET_SPAWN_HPP
