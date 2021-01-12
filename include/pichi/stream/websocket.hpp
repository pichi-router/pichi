#ifndef PICHI_STREAM_WEBSOCKET_HPP
#define PICHI_STREAM_WEBSOCKET_HPP

#include <algorithm>
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace pichi::stream {

namespace detail {

template <size_t...> struct Sequence {
};

template <size_t N, size_t... Seq> struct SeqGenerator : public SeqGenerator<N - 1, N - 1, Seq...> {
};

template <size_t... N> struct SeqGenerator<0, N...> {
  using Seq = Sequence<N...>;
};

template <typename... Params> struct ParamSaver {
  template <typename F, size_t... N> void invoke(F&& f, Sequence<N...>)
  {
    static_assert(std::is_invocable_v<F, Params...>);
    return std::invoke(std::forward<F>(f), std::get<N>(params_)...);
  }

  template <typename F, typename... Args> void invoke(F&& f, Args&&... args)
  {
    using Seq = typename SeqGenerator<sizeof...(Args) + sizeof...(Params)>::Seq;
    auto combination = ParamSaver<std::decay_t<Args>..., Params...>{
        std::tuple_cat(std::make_tuple(std::forward<Args>(args)...), params_)};
    return combination.invoke(std::forward<F>(f), Seq{});
  }

  std::tuple<Params...> params_;
};

template <typename... Params> auto makeParamSaver(Params&&... params)
{
  return ParamSaver<std::decay_t<Params>...>{std::make_tuple(std::forward<Params>(params)...)};
}

template <typename Handler, typename Params>
inline auto makeFail(Handler&& handler, Params&& params)
{
  return [h = std::forward<Handler>(handler),
          p = std::forward<Params>(params)](auto const& ec) mutable {
    boost::asio::post(boost::asio::get_associated_executor(h), [=]() mutable { p.invoke(h, ec); });
  };
}

template <typename Handler> inline auto makeSucceed(Handler&& handler)
{
  return [h = std::forward<Handler>(handler)](auto&&... args) mutable {
    boost::asio::post(boost::asio::get_associated_executor(h),
                      [h, p = makeParamSaver(std::forward<decltype(args)>(args)...)]() mutable {
                        p.invoke(h, boost::system::error_code{});
                      });
  };
}

template <size_t INDEX, typename Executor, typename Fail, typename Succeed, typename... Handlers>
class AsyncOperation {
private:
  static constexpr auto SIZE = sizeof...(Handlers);

  using ErrorCode = boost::system::error_code;
  using Next = AsyncOperation<INDEX + 1, Executor, Fail, Succeed, Handlers...>;
  using Tuple = std::tuple<Handlers...>;

  static_assert(SIZE > 0);
  static_assert(std::is_copy_constructible_v<Executor>);
  static_assert(std::is_copy_constructible_v<Fail>);
  static_assert(std::is_invocable_v<Fail, ErrorCode const&>);
  static_assert(std::is_copy_constructible_v<Succeed>);

  template <typename Handler, typename... Args> void invoke(Handler&& handler, Args&&... args)
  {
    static_assert(std::is_invocable_v<Handler, Args...>);
    try {
      std::invoke(handler, std::forward<Args>(args)...);
    }
    catch (boost::system::system_error const& e) {
      fail_(e.code());
    }
  }

  template <typename... Args> void next(Args&&... args)
  {
    if constexpr (INDEX < SIZE)
      invoke(std::get<INDEX>(handlers_), Next{ex_, fail_, succeed_, handlers_},
             std::forward<Args>(args)...);
    else
      invoke(succeed_, std::forward<Args>(args)...);
  }

public:
  AsyncOperation(Executor const& ex, Fail const& fail, Succeed const& succeed,
                 Tuple const& handlers)
    : ex_{ex}, fail_{fail}, succeed_{succeed}, handlers_{handlers}
  {
  }

  template <typename... Args> void operator()(ErrorCode const& ec, Args&&... args)
  {
    // TODO Maybe a graceful closing is necessary
    if (ec && ec != boost::beast::websocket::error::closed)
      fail_(ec);
    else
      next(std::forward<Args>(args)...);
  }

  void operator()() { next(); }

  template <typename... Args> void succeed(Args&&... args)
  {
    invoke(succeed_, std::forward<Args>(args)...);
  }

  auto get_executor() const { return ex_; }

private:
  Executor ex_;
  Fail fail_;
  Succeed succeed_;
  Tuple handlers_;
};

template <typename Executor, typename Fail, typename Succeed, typename... Handlers>
auto makeAsyncOperation(Executor const& ex, Fail const& fail, Succeed const& succeed,
                        Handlers const&... handlers)
{
  return AsyncOperation<0, Executor, Fail, Succeed, Handlers...>{ex, fail, succeed,
                                                                 std::make_tuple(handlers...)};
}

inline void assertTrue(bool b, boost::system::error_code const& ec)
{
  if (!b) boost::asio::detail::throw_error(ec);
}

}  // namespace detail

template <typename NextLayer> class WsStream {
private:
  static_assert(std::is_same_v<NextLayer, std::decay_t<NextLayer>>);

  using ErrorCode = boost::system::error_code;
  using Stream = boost::beast::websocket::stream<NextLayer>;
  using Buffer = boost::beast::flat_buffer;
  using Parser = boost::beast::http::request_parser<boost::beast::http::empty_body>;
  template <typename Handler, typename Signature>
  using AsyncCompletion = boost::asio::async_completion<Handler, Signature>;

  template <typename Signature, typename Handler, typename Params, typename... Handlers>
  auto initiate(Handler&& handler, Params&& defaults, Handlers&&... handlers)
  {
    auto init = boost::asio::async_completion<Handler, Signature>{handler};
    auto h = init.completion_handler;
    boost::asio::post(get_executor(),
                      detail::makeAsyncOperation(
                          get_executor(), detail::makeFail(h, std::forward<Params>(defaults)),
                          detail::makeSucceed(h), std::forward<Handlers>(handlers)...));
    return init.result.get();
  }

public:
  using executor_type = typename Stream::executor_type;
  using next_layer_type = typename Stream::next_layer_type;

  template <typename... Args>
  WsStream(std::string_view path, std::string_view host, Args&&... args)
    : path_{path.data(), path.size()},
      host_{host.data(), host.size()},
      delegate_{std::forward<Args>(args)...},
      buf_{},
      parser_{}
  {
  }

  auto get_executor() { return delegate_.get_executor(); }

  bool is_open() const { return delegate_.is_open(); }

  auto& next_layer() { return delegate_.next_layer(); }
  auto const& next_layer() const { return delegate_.next_layer(); }

  template <typename HandshakeHandler> auto async_handshake(HandshakeHandler&& handler)
  {
    return delegate_.async_handshake(host_, path_, std::forward<HandshakeHandler>(handler));
  }

  template <typename AcceptHandler> auto async_accept(AcceptHandler&& handler)
  {
    return initiate<void(ErrorCode const&)>(
        std::forward<AcceptHandler>(handler), detail::makeParamSaver(),
        [this](auto&& next) {
          boost::beast::http::async_read_header(delegate_.next_layer(), buf_, parser_, next);
        },
        [this](auto&& next, auto) {
          auto header = parser_.release();
          detail::assertTrue(header.target() == path_, boost::beast::http::error::bad_target);
          detail::assertTrue(header[boost::beast::http::field::host] == host_,
                             boost::beast::http::error::bad_value);
          delegate_.async_accept(header, next);
        });
  }

  template <typename ShutdownHandler> auto async_shutdown(ShutdownHandler&& handler)
  {
    return delegate_.async_close(boost::beast::websocket::normal,
                                 std::forward<ShutdownHandler>(handler));
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  auto async_read_some(MutableBufferSequence const& buf, ReadHandler&& handler)
  {
    return initiate<void(ErrorCode const&, size_t)>(
        std::forward<ReadHandler>(handler), detail::makeParamSaver(0_sz), [this, buf](auto&& next) {
          if (buf_.size() == 0) {
            delegate_.async_read_some(buf, next);
            return;
          }
          auto copied = std::min(buf_.size(), boost::asio::buffer_size(buf));
          boost::asio::buffer_copy(buf_.data(), buf);
          buf_.consume(copied);
          next.succeed(copied);
        });
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  auto async_write_some(ConstBufferSequence const& buf, WriteHandler&& handler)
  {
    delegate_.binary(true);
    return delegate_.async_write_some(true, buf, std::forward<WriteHandler>(handler));
  }

private:
  std::string path_;
  std::string host_;
  Stream delegate_;
  Buffer buf_;
  Parser parser_;
};

template <typename NextLayer> struct RawStream<WsStream<NextLayer>> : public std::false_type {
};

template <typename Socket> class TlsStream;

}  // namespace pichi::stream

namespace boost::asio {

template <size_t INDEX, typename Executor, typename Fail, typename Succeed, typename... Handlers>
inline bool asio_handler_is_continuation(
    pichi::stream::detail::AsyncOperation<INDEX, Executor, Fail, Succeed, Handlers...>*)
{
  return true;
}

template <size_t INDEX, typename Executor, typename Fail, typename Succeed, typename... Handlers>
struct associated_executor<
    pichi::stream::detail::AsyncOperation<INDEX, Executor, Fail, Succeed, Handlers...>> {
  using type = executor;

  static type get(
      pichi::stream::detail::AsyncOperation<INDEX, Executor, Fail, Succeed, Handlers...> const& h,
      executor const& = executor{}) BOOST_ASIO_NOEXCEPT
  {
    return h.get_executor();
  }
};

}  // namespace boost::asio

namespace boost::beast::websocket {

template <class Socket, class TeardownHandler>
void async_teardown(role_type, pichi::stream::TlsStream<Socket>& stream, TeardownHandler&& handler)
{
  stream.async_shutdown(std::forward<TeardownHandler>(handler));
}

}  // namespace boost::beast::websocket

#endif  // PICHI_STREAM_WEBSOCKET_HPP
