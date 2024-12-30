#ifndef PICHI_STREAM_HELPERS_HPP
#define PICHI_STREAM_HELPERS_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/asio/write.hpp>
#include <concepts>
#include <functional>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/stream/completer.hpp>

namespace pichi::stream {

template <typename Object>
concept LayeredObject = requires(Object obj) {
  typename Object::next_layer_type;
  { obj.next_layer() } -> std::same_as<typename Object::next_layer_type&>;
  { std::as_const(obj).next_layer() } -> std::same_as<typename Object::next_layer_type const&>;
};

template <typename Stream>
concept AsyncReadable = requires(
    Stream stream, boost::asio::mutable_buffer mb, boost::asio::yield_context yield,
    std::function<void(boost::system::error_code, size_t)> callback
) {
  { stream.async_read_some(mb, boost::asio::use_awaitable) } -> std::same_as<Awaitable<size_t>>;
  { stream.async_read_some(mb, yield) } -> std::same_as<size_t>;
  { stream.async_read_some(mb, callback) } -> std::same_as<void>;
};

template <typename Stream>
concept AsyncWritable = requires(
    Stream stream, boost::asio::const_buffer cb, boost::asio::yield_context yield,
    std::function<void(boost::system::error_code, size_t)> callback
) {
  { stream.async_write_some(cb, boost::asio::use_awaitable) } -> std::same_as<Awaitable<size_t>>;
  { stream.async_write_some(cb, yield) } -> std::same_as<size_t>;
  { stream.async_write_some(cb, callback) } -> std::same_as<void>;
};

template <typename Stream>
concept HandshakeBase = requires(
    Stream stream, boost::asio::yield_context yield,
    std::function<void(boost::system::error_code)> callback
) {
  { stream.async_shutdown(boost::asio::use_awaitable) } -> std::same_as<Awaitable<void>>;
  { stream.async_shutdown(yield) } -> std::same_as<void>;
  { stream.async_shutdown(callback) } -> std::same_as<void>;
};

template <typename Stream>
concept HandshakeClient =
    HandshakeBase<Stream> && requires(
                                 Stream stream, boost::asio::yield_context yield,
                                 std::function<void(boost::system::error_code)> callback
                             ) {
      { stream.async_handshake(boost::asio::use_awaitable) } -> std::same_as<Awaitable<void>>;
      { stream.async_handshake(yield) } -> std::same_as<void>;
      { stream.async_handshake(callback) } -> std::same_as<void>;
    };

template <typename Stream>
concept HandshakeServer =
    HandshakeBase<Stream> && requires(
                                 Stream stream, boost::asio::yield_context yield,
                                 std::function<void(boost::system::error_code)> callback
                             ) {
      { stream.async_accept(boost::asio::use_awaitable) } -> std::same_as<Awaitable<void>>;
      { stream.async_accept(yield) } -> std::same_as<void>;
      { stream.async_accept(callback) } -> std::same_as<void>;
    };

inline Awaitable<void> close(boost::asio::ip::tcp::socket& s)
{
  s.close();
  co_return;
}

template <typename Stream>
requires(LayeredObject<Stream> && HandshakeBase<Stream>)
Awaitable<void> close(Stream& stream)
{
  co_await stream.async_shutdown(boost::asio::use_awaitable);
  co_await close(stream.next_layer());
}

inline Awaitable<void> connect(boost::asio::ip::tcp::socket& s, Endpoint const& peer)
{
  if (peer.type_ == EndpointType::DOMAIN_NAME) {
    co_await boost::asio::async_connect(
        s,
        co_await boost::asio::ip::tcp::resolver{s.get_executor()}
            .async_resolve(peer.host_, std::to_string(peer.port_), boost::asio::use_awaitable),
        boost::asio::use_awaitable
    );
  }
  else {
    co_await s.async_connect(
        {boost::asio::ip::make_address(peer.host_), peer.port_},
        boost::asio::use_awaitable
    );
  }
}

template <typename Stream>
requires(LayeredObject<Stream> && HandshakeClient<Stream>)
Awaitable<void> connect(Stream& stream, Endpoint const& peer)
{
  co_await connect(stream.next_layer(), peer);
  co_await stream.async_handshake(boost::asio::use_awaitable);
}

inline Awaitable<void> accept(boost::asio::ip::tcp::socket&) { co_return; }

template <typename Stream>
requires(LayeredObject<Stream> && HandshakeServer<Stream>)
Awaitable<void> accept(Stream& stream)
{
  co_await accept(stream.next_layer());
  co_await stream.async_accept(boost::asio::use_awaitable);
}

Awaitable<void> read(AsyncReadable auto& stream, MutableBuffer buf)
{
  co_await boost::asio::async_read(stream, boost::asio::buffer(buf), boost::asio::use_awaitable);
}

Awaitable<size_t> read_some(AsyncReadable auto& stream, MutableBuffer buf)
{
  co_return co_await stream.async_read_some(boost::asio::buffer(buf), boost::asio::use_awaitable);
}

Awaitable<void> write(AsyncWritable auto& stream, ConstBuffer buf)
{
  co_await boost::asio::async_write(stream, boost::asio::buffer(buf), boost::asio::use_awaitable);
}

Awaitable<size_t> write_some(AsyncWritable auto& stream, ConstBuffer buf)
{
  co_return co_await stream.async_write_some(boost::asio::buffer(buf), boost::asio::use_awaitable);
}

template <
    typename Signature, typename CompletionToken, boost::asio::execution::executor Executor,
    typename Func, typename... Args>
requires(std::invocable<Func, Args...>)
auto async_initiate(Executor const& ex, CompletionToken&& token, Func&& f, Args&&... args)
{
  return boost::asio::async_initiate<CompletionToken, Signature>(
      [ex](auto&& h, auto&& f, auto&&... args) {
        boost::asio::co_spawn(
            ex,
            std::invoke(std::forward<Func>(f), std::forward<Args>(args)...),
            Completer{std::move(h)}
        );
      },
      token,
      std::forward<Func>(f),
      std::forward<Args>(args)...
  );
}

template <typename Signature, typename CompletionToken, typename... Args>
requires(!boost::asio::execution::executor<CompletionToken>)
auto async_initiate(CompletionToken&& token, Args&&... args)
{
  return boost::asio::async_initiate<CompletionToken, Signature>(
      [](auto&& h, auto&&... args) {
        auto ex = boost::asio::get_associated_executor(h);
        boost::asio::dispatch(
            ex,
            [h = std::move(h), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
              std::apply(h, std::move(args));
            }
        );
      },
      token,
      std::forward<Args>(args)...
  );
}

}  // namespace pichi::stream

#endif  // PICHI_STREAM_HELPERS_HPP
