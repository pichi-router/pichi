#ifndef PICHI_STREAM_OPERATION_HPP
#define PICHI_STREAM_OPERATION_HPP

#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/post.hpp>
#include <stdint.h>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pichi::stream {

namespace detail {

template <typename Tuple, size_t... Is> auto pop(Tuple&& t, std::index_sequence<Is...>)
{
  return std::make_tuple(std::get<Is + 1>(std::forward<Tuple>(t))...);
}

template <typename Tuple> auto pop(Tuple&& t)
{
  return pop(std::forward<Tuple>(t),
             std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>> - 1>{});
}

}  // namespace detail

template <typename Executor, typename Completor, typename OnFailure, typename Handlers>
class AsyncOperation;

template <typename Executor, typename Completor, typename OnFailure, typename Handlers>
auto makeOperation(Executor const& ex, Completor const& c, OnFailure const& f, Handlers&& h)
{
  using Op = AsyncOperation<Executor, Completor, OnFailure, std::decay_t<Handlers>>;
  return Op{ex, c, f, std::forward<Handlers>(h)};
}

template <typename CompletionHandler> class Completor {
public:
  Completor(CompletionHandler const& h) : h_{h} {}

  template <typename... Args> void operator()(Args&&... args)
  {
    static_assert(std::is_invocable_v<CompletionHandler, Args...>);
    boost::asio::post(boost::asio::get_associated_executor(h_),
                      [h = h_, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                        std::apply(h, std::move(args));
                      });
  }

private:
  CompletionHandler h_;
};

template <typename Executor, typename Completor, typename OnFailure, typename Handlers>
class AsyncOperation {
private:
  static_assert(std::is_copy_constructible_v<Completor>);
  static_assert(std::is_copy_constructible_v<OnFailure>);
  static_assert(std::is_same_v<Handlers, std::decay_t<Handlers>>);
  static_assert(std::is_copy_constructible_v<Handlers>);

  using ErrorCode = boost::system::error_code;

  template <typename... Args> void next(Args&&... args)
  {
    if constexpr (std::tuple_size_v<Handlers> > 0) {
      try {
        std::invoke(std::get<0>(handlers_),
                    makeOperation(ex_, complete_, fail_, detail::pop(std::move(handlers_))),
                    std::forward<Args>(args)...);
      }
      catch (boost::system::system_error const& e) {
        fail_(complete_, e.code());
      }
    }
    else {
      succeed(std::forward<Args>(args)...);
    }
  }

public:
  template <typename Hs>
  AsyncOperation(Executor const& ex, Completor const& complete, OnFailure const& fail,
                 Hs&& handlers)
    : ex_{ex}, complete_{complete}, fail_{fail}, handlers_{std::forward<Hs>(handlers)}
  {
  }

  Executor get_executor() const { return ex_; }

  template <typename... Args> void operator()(ErrorCode const& ec, Args&&... args)
  {
    if (ec) {
      fail_(complete_, ec);
    }
    else {
      next(std::forward<Args>(args)...);
    }
  }

  void operator()() { std::invoke(*this, ErrorCode{}); }

  template <typename... Args> void succeed(Args&&... args)
  {
    complete_(ErrorCode{}, std::forward<Args>(args)...);
  }

private:
  Executor ex_;
  Completor complete_;
  OnFailure fail_;
  Handlers handlers_;
};

template <typename Signature, typename Executor, typename CompletionToken, typename OnFailure,
          typename... Handlers>
auto initiate(Executor const& ex, CompletionToken&& token, OnFailure const& fail,
              Handlers&&... handlers)
{
  auto init = boost::asio::async_completion<CompletionToken, Signature>{token};
  boost::asio::post(makeOperation(ex, Completor{init.completion_handler}, fail,
                                  std::make_tuple(std::forward<Handlers>(handlers)...)));
  return init.result.get();
}

}  // namespace pichi::stream

namespace boost::asio {

template <typename Executor, typename Completor, typename OnFailure, typename Handlers,
          typename OtherExecutor>
struct associated_executor<pichi::stream::AsyncOperation<Executor, Completor, OnFailure, Handlers>,
                           OtherExecutor> {
  using type = Executor;

  static type get(pichi::stream::AsyncOperation<Executor, Completor, OnFailure, Handlers> const& h,
                  OtherExecutor const& = OtherExecutor{}) BOOST_ASIO_NOEXCEPT
  {
    return h.get_executor();
  }
};

}  // namespace boost::asio

#endif  // PICHI_STREAM_OPERATION_HPP
