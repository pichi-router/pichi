#ifndef PICHI_COMMON_CORO_HPP
#define PICHI_COMMON_CORO_HPP

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/outstanding_work.hpp>
#include <boost/asio/prefer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <concepts>
#include <optional>
#include <tuple>

namespace pichi {

using IOExecutor = boost::asio::any_io_executor;

template <typename T, boost::asio::execution::executor E = IOExecutor>
using Awaitable = boost::asio::awaitable<T, E>;

template <typename Context>
concept ExecutionContext = requires(Context ctx) {
  { ctx.get_executor() } -> boost::asio::execution::executor;
  { ctx.get_executor() } -> std::same_as<typename Context::executor_type>;
};

template <boost::asio::execution::executor Executor> Awaitable<void> switch_to(Executor const& ex)
{
  co_await boost::asio::dispatch(boost::asio::bind_executor(ex, boost::asio::use_awaitable));
}

template <typename T, boost::asio::execution::executor E>
requires(!std::is_same_v<T, void>)
Awaitable<std::optional<T>, E> redirect(Awaitable<T, E> a, boost::system::error_code& ec)
{
  try {
    auto f = boost::asio::detail::awaitable_as_function<T, E>{std::move(a)};
    auto r = std::make_optional(co_await f());
    ec     = {};
    co_return r;
  }
  catch (boost::system::system_error const& e) {
    ec = e.code();
    co_return std::nullopt;
  }
}

template <boost::asio::execution::executor E>
Awaitable<void, E> redirect(Awaitable<void, E> a, boost::system::error_code& ec)
{
  try {
    auto f = boost::asio::detail::awaitable_as_function<void, E>{std::move(a)};
    co_await f();
    ec = {};
  }
  catch (boost::system::system_error const& e) {
    ec = e.code();
  }
}

template <typename T, boost::asio::execution::executor E>
requires(!std::is_same_v<T, void>)
Awaitable<std::tuple<boost::system::error_code, std::optional<T>>, E> redirect(Awaitable<T, E> a)
{
  auto ec = boost::system::error_code{};
  auto rt = co_await redirect(std::move(a), ec);
  co_return std::make_tuple(ec, std::move(rt));
}

template <boost::asio::execution::executor E>
Awaitable<boost::system::error_code, E> redirect(Awaitable<void, E> a)
{
  auto ec = boost::system::error_code{};
  co_await redirect(std::move(a), ec);
  co_return ec;
}

template <typename T, boost::asio::execution::executor E, boost::asio::execution::executor Executor>
requires(!std::is_same_v<T, void>)
Awaitable<T, E> exec_to(Executor const& executor, Awaitable<T, E> a)
{
  auto ex     = boost::asio::prefer(executor, boost::asio::execution::outstanding_work.tracked);
  auto [e, r] = co_await redirect(std::move(a));
  co_await switch_to(ex);
  if (e)
    throw boost::system::system_error{e};
  else
    co_return *r;
}

template <boost::asio::execution::executor E, boost::asio::execution::executor Executor>
Awaitable<void, E> exec_to(Executor const& executor, Awaitable<void, E> a)
{
  auto ex = boost::asio::prefer(executor, boost::asio::execution::outstanding_work.tracked);
  auto ec = co_await redirect(std::move(a));
  co_await switch_to(ex);
  if (ec) throw boost::system::system_error{ec};
}

}  // namespace pichi

#endif  // PICHI_COMMON_CORO_HPP
