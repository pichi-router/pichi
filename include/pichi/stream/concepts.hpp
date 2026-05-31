#ifndef PICHI_STREAM_CONCEPTS_HPP
#define PICHI_STREAM_CONCEPTS_HPP

#include <boost/asio/buffer.hpp>
#include <concepts>
#include <pichi/common/coro.hpp>
#include <utility>

namespace pichi::stream {

template <typename Object>
concept Layered = requires(Object obj) {
  typename Object::next_layer_type;
  { obj.next_layer() } -> std::same_as<typename Object::next_layer_type&>;
  { std::as_const(obj).next_layer() } -> std::same_as<typename Object::next_layer_type const&>;
};

template <typename Object>
concept Closable = requires(Object obj) {
  obj.close();
  { std::as_const(obj).is_open() } -> std::same_as<bool>;
};

template <typename Object>
concept Connectable = requires(
    Object obj, typename Object::endpoint_type peer,
    std::function<void(boost::system::error_code)> callback
) {
  { obj.async_connect(peer, boost::asio::use_awaitable) } -> std::same_as<Awaitable<void>>;
  { obj.async_connect(peer, callback) } -> std::same_as<void>;
};

template <typename Stream>
concept AsyncReadable = requires(
    Stream stream, boost::asio::mutable_buffer mb,
    std::function<void(boost::system::error_code, size_t)> callback
) {
  { stream.async_read_some(mb, boost::asio::use_awaitable) } -> std::same_as<Awaitable<size_t>>;
  { stream.async_read_some(mb, callback) } -> std::same_as<void>;
};

template <typename Stream>
concept AsyncWritable = requires(
    Stream stream, boost::asio::const_buffer cb,
    std::function<void(boost::system::error_code, size_t)> callback
) {
  { stream.async_write_some(cb, boost::asio::use_awaitable) } -> std::same_as<Awaitable<size_t>>;
  { stream.async_write_some(cb, callback) } -> std::same_as<void>;
};

template <typename Stream>
concept Shutdownable =
    requires(Stream stream, std::function<void(boost::system::error_code)> callback) {
      { stream.async_shutdown(boost::asio::use_awaitable) } -> std::same_as<Awaitable<void>>;
      { stream.async_shutdown(callback) } -> std::same_as<void>;
    };

template <typename Stream>
concept Handshakable =
    requires(Stream stream, std::function<void(boost::system::error_code)> callback) {
      { stream.async_handshake(boost::asio::use_awaitable) } -> std::same_as<Awaitable<void>>;
      { stream.async_handshake(callback) } -> std::same_as<void>;
    };

template <typename Stream>
concept Acceptable =
    requires(Stream stream, std::function<void(boost::system::error_code)> callback) {
      { stream.async_accept(boost::asio::use_awaitable) } -> std::same_as<Awaitable<void>>;
      { stream.async_accept(callback) } -> std::same_as<void>;
    };

template <typename Socket>
concept AsyncSocket = Closable<Socket> && Connectable<Socket> && AsyncReadable<Socket> &&
                      AsyncWritable<Socket> && requires(Socket s) {
                        {
                          s.get_executor()
                        } -> std::constructible_from<typename Socket::executor_type>;
                        { s.get_executor() } -> boost::asio::execution::executor;
                      };

// TODO Rename it after deprecating pichi::net
template <typename Stream>
concept AsyncStream =
    Layered<Stream> && Shutdownable<Stream> && AsyncReadable<Stream> && AsyncWritable<Stream>;

template <typename Layer>
concept AsyncLayer = AsyncSocket<Layer> || AsyncStream<Layer>;

}  // namespace pichi::stream

#endif  // PICHI_STREAM_CONCEPTS_HPP
