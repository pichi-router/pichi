#ifndef PICHI_STREAM_TLS_HPP
#define PICHI_STREAM_TLS_HPP

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <pichi/stream/traits.hpp>
#include <type_traits>

namespace pichi::stream {

/*
 *  1. TlsStream is about to implement both of AsyncReadStream and
 * AsyncWriteStream concepts, which is required by the HTTP functions provided
 * by Boost.Beast.
 *  2. The main difference bewteen TlsStream and boost::asio::ssl::stream is
 * TlsStream keeps boost::asio::ssl::context in its scope.
 */

template <typename Socket> class TlsStream {
private:
  static_assert(std::is_same_v<std::decay_t<Socket>, Socket>);

  using Context = boost::asio::ssl::context;
  using Stream = boost::asio::ssl::stream<Socket>;
  using ErrorCode = boost::system::error_code;

public:
  using executor_type = typename Stream::executor_type;

  template <typename... Args>
  TlsStream(Context&& ctx, Args&&... args)
    : ctx_{std::move(ctx)}, stream_{Socket{std::forward<Args>(args)...}, ctx_}
  {
  }

  template <typename... Args>
  TlsStream(std::optional<std::string> const& sni, Args&&... args)
    : TlsStream{std::forward<Args>(args)...}
  {
    if (sni.has_value())
      assertTrue(SSL_set_tlsext_host_name(stream_.native_handle(), sni->c_str()) == 1);
  }

  auto get_executor() { return stream_.get_executor(); }

  bool is_open() const { return stream_.next_layer().is_open(); }

  template <typename ConnectHandler>
  auto async_connect(typename Socket::endpoint_type const& peer, ConnectHandler&& handler)
  {
    return stream_.next_layer().async_connect(peer, std::forward<ConnectHandler>(handler));
  }

  template <typename HandshakeHandler> auto async_handshake(HandshakeHandler&& handler)
  {
    return stream_.async_handshake(boost::asio::ssl::stream_base::client,
                                   std::forward<HandshakeHandler>(handler));
  }

  template <typename AcceptHandler> auto async_accept(AcceptHandler&& handler)
  {
    return stream_.async_handshake(boost::asio::ssl::stream_base::server,
                                   std::forward<AcceptHandler>(handler));
  }

  template <typename ShutdownHandler> auto async_shutdown(ShutdownHandler&& handler)
  {
    return stream_.async_shutdown(std::forward<ShutdownHandler>(handler));
  }

  void close(ErrorCode& ec) { stream_.next_layer().close(ec); }

  template <typename MutableBufferSequence, typename ReadHandler>
  auto async_read_some(MutableBufferSequence const& buf, ReadHandler&& handler)
  {
    return stream_.async_read_some(buf, std::forward<ReadHandler>(handler));
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  auto async_write_some(ConstBufferSequence const& buf, WriteHandler&& handler)
  {
    static_assert(std::is_invocable_v<std::decay_t<WriteHandler>, ErrorCode const&, size_t>);
    return stream_.async_write_some(buf, std::forward<WriteHandler>(handler));
  }

private:
  Context ctx_;
  Stream stream_;
};

template <typename Socket> struct RawStream<TlsStream<Socket>> : public std::false_type {
};

}  // namespace pichi::stream

#endif  // PICHI_STREAM_TLS_HPP
