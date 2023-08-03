#ifndef PICHI_STREAM_WEBSOCKET_HPP
#define PICHI_STREAM_WEBSOCKET_HPP

#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <optional>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/operation.hpp>
#include <string>
#include <string_view>
#include <type_traits>

namespace pichi::stream {

template <typename NextLayer> class WsStream {
private:
  static_assert(std::is_same_v<NextLayer, std::decay_t<NextLayer>>);

  using ErrorCode = boost::system::error_code;
  using Stream = boost::beast::websocket::stream<NextLayer>;
  using Buffer = boost::beast::flat_buffer;
  using Header = boost::beast::http::request<boost::beast::http::empty_body>;
  using Parser = boost::beast::http::request_parser<boost::beast::http::empty_body>;

  template <typename Handler, typename Signature>
  using AsyncCompletion = boost::asio::async_completion<Handler, Signature>;

  template <typename Completor, typename... Args>
  void onFailure(Completor& complete, ErrorCode const& ec, Args&&... args)
  {
    auto err = Buffer{};
    if (header_.has_value()) boost::beast::ostream(err) << *header_;
    if (buf_.size() > 0) {
      boost::asio::buffer_copy(err.prepare(buf_.size()), buf_.cdata());
      err.commit(buf_.size());
    }
    std::swap(err, buf_);
    complete(ec, std::forward<Args>(args)...);
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
      parser_{},
      header_{}
  {
  }

  auto releaseBuffer() { return std::move(buf_); }

  auto get_executor() { return delegate_.get_executor(); }

  bool is_open() const { return delegate_.is_open(); }

  auto& next_layer() { return delegate_.next_layer(); }
  auto const& next_layer() const { return delegate_.next_layer(); }

  template <typename HandshakeHandler> auto async_handshake(HandshakeHandler&& handler)
  {
    return delegate_.async_handshake(host_, path_, std::forward<HandshakeHandler>(handler));
  }

  template <typename AcceptToken> auto async_accept(AcceptToken&& token)
  {
    return initiate<void(ErrorCode const&)>(
        get_executor(), std::forward<AcceptToken>(token),
        [this](auto& complete, auto&& ec) { this->onFailure(complete, ec); },
        [this](auto&& next) {
          boost::beast::http::async_read_header(delegate_.next_layer(), buf_, parser_, next);
        },
        [this](auto&& next, auto) {
          header_ = parser_.release();
          assertTrue(header_->target() == path_, boost::beast::http::error::bad_target);
          if (!host_.empty())
            assertTrue((*header_)[boost::beast::http::field::host] == host_,
                       boost::beast::http::error::bad_value);
          delegate_.async_accept(*header_, next);
        },
        [this](auto&& next) {
          header_.reset();
          next.succeed();
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
        get_executor(), std::forward<ReadHandler>(handler),
        [this](auto& complete, auto&& ec) { this->onFailure(complete, ec, 0_sz); },
        [this, buf](auto&& next) {
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
  std::optional<Header> header_;
};

template <typename NextLayer> struct RawStream<WsStream<NextLayer>> : public std::false_type {};

template <typename Socket> class TlsStream;

}  // namespace pichi::stream

namespace boost::beast::websocket {

template <class Socket, class TeardownHandler>
void async_teardown(role_type, pichi::stream::TlsStream<Socket>& stream, TeardownHandler&& handler)
{
  stream.async_shutdown(std::forward<TeardownHandler>(handler));
}

}  // namespace boost::beast::websocket

#endif  // PICHI_STREAM_WEBSOCKET_HPP
