#ifndef PICHI_STREAM_TLS_HPP
#define PICHI_STREAM_TLS_HPP

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>

namespace pichi::stream {

extern boost::asio::ssl::context tls_context(vo::TlsIngressOption const&);
extern boost::asio::ssl::context tls_context(vo::TlsEgressOption const&, std::string const&);

extern void setup_fingerprint(::SSL*);

/*
 *  1. Tls is about to implement both of AsyncReadStream and
 * AsyncWriteStream concepts, which is required by the HTTP functions provided
 * by Boost.Beast.
 *  2. The main difference bewteen TlsStream and boost::asio::ssl::stream is
 * TlsStream keeps boost::asio::ssl::context in its scope.
 */

template <typename Socket> class Tls {
private:
  using Context   = boost::asio::ssl::context;
  using Stream    = boost::asio::ssl::stream<Socket>;
  using ErrorCode = boost::system::error_code;

public:
  using executor_type   = typename Stream::executor_type;
  using next_layer_type = typename Stream::next_layer_type;

  template <typename... Args>
  Tls(Context ctx, Args&&... args)
    : ctx_{std::move(ctx)}, stream_{std::forward<Args>(args)..., ctx_}
  {
  }

  template <typename... Args>
  Tls(std::optional<std::string> const& sni, Args&&... args) : Tls{std::forward<Args>(args)...}
  {
    if (sni.has_value())
      assertTrue(SSL_set_tlsext_host_name(stream_.native_handle(), sni->c_str()) == 1);
    setup_fingerprint(stream_.native_handle());
  }

  executor_type get_executor() { return stream_.get_executor(); }

  next_layer_type&       next_layer() { return stream_.next_layer(); }
  next_layer_type const& next_layer() const { return stream_.next_layer(); }

  bool is_open() const { return stream_.next_layer().is_open(); }

  template <typename ShutdownToken> auto async_shutdown(ShutdownToken&& token)
  {
    return stream_.async_shutdown(std::forward<ShutdownToken>(token));
  }

  template <typename HandshakeToken> auto async_handshake(HandshakeToken&& token)
  {
    return stream_.async_handshake(
        boost::asio::ssl::stream_base::client,
        std::forward<HandshakeToken>(token)
    );
  }

  template <typename AcceptToken> auto async_accept(AcceptToken&& token)
  {
    return stream_.async_handshake(
        boost::asio::ssl::stream_base::server,
        std::forward<AcceptToken>(token)
    );
  }

  template <typename MutableBufferSequence, typename ReadToken>
  auto async_read_some(MutableBufferSequence const& b, ReadToken&& token)
  {
    return stream_.async_read_some(b, std::forward<ReadToken>(token));
  }

  template <typename ConstBufferSequence, typename WriteToken>
  auto async_write_some(ConstBufferSequence const& b, WriteToken&& token)
  {
    return stream_.async_write_some(b, std::forward<WriteToken>(token));
  }

private:
  Context ctx_;
  Stream  stream_;
};

}  // namespace pichi::stream

#endif  // PICHI_STREAM_TLS_HPP
