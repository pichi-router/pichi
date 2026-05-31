#ifndef PICHI_STREAM_WEBSOCKET_HPP
#define PICHI_STREAM_WEBSOCKET_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/helpers.hpp>
#include <pichi/stream/tls.hpp>
#include <string>
#include <string_view>

namespace pichi::stream {

template <typename NextLayer> class Websocket {
private:
  using ErrorCode = boost::system::error_code;
  using Stream    = boost::beast::websocket::stream<NextLayer>;
  using Buffer    = boost::beast::flat_buffer;
  using Header    = boost::beast::http::request<boost::beast::http::empty_body>;
  using Parser    = boost::beast::http::request_parser<boost::beast::http::empty_body>;

  Awaitable<void> do_accept()
  {
    auto parser = Parser{};
    co_await boost::beast::http::async_read_header(
        delegate_.next_layer(),
        buf_,
        parser,
        boost::asio::use_awaitable
    );
    auto header = parser.release();
    assertTrue(header.target() == path_, boost::beast::http::error::bad_target);
    if (!host_.empty())
      assertTrue(
          header[boost::beast::http::field::host] == host_,
          boost::beast::http::error::bad_value
      );
    co_await delegate_.async_accept(header, boost::asio::use_awaitable);
  }

  template <typename MutableBufferSequence>
  Awaitable<size_t> do_read(MutableBufferSequence const& b)
  {
    if (buf_.size() == 0) {
      co_return co_await delegate_.async_read_some(b, boost::asio::use_awaitable);
    }

    auto copied = std::min(buf_.size(), boost::asio::buffer_size(b));
    boost::asio::buffer_copy(buf_.data(), b);
    buf_.consume(copied);
    co_return copied;
  }

public:
  using executor_type   = typename Stream::executor_type;
  using next_layer_type = typename Stream::next_layer_type;

  template <typename... Args>
  Websocket(std::string_view path, std::string_view host, Args&&... args)
    : path_{path.data(), path.size()},
      host_{host.data(), host.size()},
      delegate_{std::forward<Args>(args)...}
  {
  }

  executor_type get_executor() { return delegate_.get_executor(); }

  next_layer_type&       next_layer() { return delegate_.next_layer(); }
  next_layer_type const& next_layer() const { return delegate_.next_layer(); }

  bool is_open() const { return delegate_.is_open(); }

  template <typename ShutdownToken> auto async_shutdown(ShutdownToken&& token)
  {
    return delegate_.async_close(
        boost::beast::websocket::normal,
        std::forward<ShutdownToken>(token)
    );
  }

  template <typename HandshakeToken> auto async_handshake(HandshakeToken&& token)
  {
    return delegate_.async_handshake(host_, path_, std::forward<HandshakeToken>(token));
  }

  template <typename AcceptToken> auto async_accept(AcceptToken&& token)
  {
    return pichi::stream::async_initiate<void(ErrorCode const&), AcceptToken, executor_type>(
        get_executor(),
        std::forward<AcceptToken>(token),
        [this]() { return do_accept(); }
    );
  }

  template <typename MutableBufferSequence, typename ReadToken>
  auto async_read_some(MutableBufferSequence const& b, ReadToken&& token)
  {
    return pichi::stream::async_initiate<void(ErrorCode const&, size_t), ReadToken, executor_type>(
        get_executor(),
        std::forward<ReadToken>(token),
        [this](auto&& b) { return this->do_read(b); },
        b
    );
  }

  template <typename ConstBufferSequence, typename WriteToken>
  auto async_write_some(ConstBufferSequence const& b, WriteToken&& token)
  {
    delegate_.binary(true);
    return delegate_.async_write_some(true, b, std::forward<WriteToken>(token));
  }

private:
  std::string path_;
  std::string host_;
  Stream      delegate_;
  Buffer      buf_ = {};
};

}  // namespace pichi::stream

namespace boost::beast::websocket {

template <typename Socket, typename TeardownToken>
void async_teardown(role_type, pichi::stream::Tls<Socket>& stream, TeardownToken&& token)
{
  stream.async_shutdown(std::forward<TeardownToken>(token));
}

}  // namespace boost::beast::websocket

#endif  // PICHI_STREAM_WEBSOCKET_HPP
