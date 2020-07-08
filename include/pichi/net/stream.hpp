#ifndef PICHI_NET_STREAM_HPP
#define PICHI_NET_STREAM_HPP

#include <pichi/config.hpp>

#include <algorithm>
#include <boost/asio/buffers_iterator.hpp>
#include <functional>
#include <pichi/asserts.hpp>
#include <pichi/buffer.hpp>
#include <pichi/net/adapter.hpp>
#include <type_traits>
#include <utility>

#ifdef ENABLE_TLS
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#endif // ENABLE_TLS

#ifdef BUILD_TEST
#include <boost/beast/core/flat_buffer.hpp>
#include <deque>
#endif // BUILD_TEST

namespace pichi::net {

namespace detail {

template <typename Buffer, typename OutputIt>
size_t readFromBuffer(Buffer& buf, OutputIt it, size_t n)
{
  std::copy_n(boost::asio::buffers_begin(buf.data()), n, it);
  buf.consume(n);
  return n;
}

template <typename Buffer, typename InputIt> size_t writeToBuffer(Buffer& buf, InputIt it, size_t n)
{
  std::copy_n(it, n, boost::asio::buffers_begin(buf.prepare(n)));
  buf.commit(n);
  return n;
}

// TODO Boost.Asio provides async_initiate function template from 1.70.0.
template <typename Signature, typename Initiation, typename CompletionToken, typename... Args>
auto asyncInitiate(Initiation&& initiation, CompletionToken&& token, Args&&... args)
{
  auto t = std::forward<CompletionToken>(token);
  auto init = boost::asio::async_completion<std::decay_t<CompletionToken>, Signature>{t};
  std::invoke(std::forward<Initiation>(initiation), init.completion_handler,
              std::forward<Args>(args)...);
  return init.result.get();
}

template <typename Stream, typename Iterator, typename ConnectHandler>
struct IteratorConnectOperator {
  IteratorConnectOperator(Stream& s, Iterator it, ConnectHandler const& h) : s_{s}, it_{it}, h_{h}
  {
  }

  void operator()(boost::system::error_code ec)
  {
    if (!ec) {
      boost::asio::post(boost::asio::get_associated_executor(h_),
                        [h = h_, ec]() mutable { std::invoke(h, ec); });
      return;
    }
    if (++it_ == Iterator{}) {
      boost::asio::post(boost::asio::get_associated_executor(h_),
                        [h = h_, ec]() mutable { std::invoke(h, ec); });
      return;
    }
    s_.close(ec);
    s_.async_connect(*it_, IteratorConnectOperator(s_, it_, h_));
  }

  Stream& s_;
  Iterator it_;
  ConnectHandler h_;
};

} // namespace detail

template <typename T> struct IsTlsStream : public std::false_type {
};

template <typename T> inline constexpr bool IsTlsStreamV = IsTlsStream<T>::value;

#ifdef ENABLE_TLS

/*
 *  1. TlsStream is about to implement both of AsyncReadStream and AsyncWriteStream concepts,
 *     which is required by the HTTP functions provided by Boost.Beast.
 *  2. The main difference bewteen TlsStream and boost::asio::ssl::stream is TlsStream keeps
 *     boost::asio::ssl::context in its scope.
 */

template <typename Socket> class TlsStream {
private:
  static_assert(std::is_same_v<std::decay_t<Socket>, Socket>);

  using Yield = boost::asio::yield_context;
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

  auto get_executor() { return stream_.get_executor(); }

  bool is_open() const { return stream_.next_layer().is_open(); }

  template <typename ConnectHandler>
  auto async_connect(typename Socket::endpoint_type const& peer, ConnectHandler&& handler)
  {
    return stream_.next_layer().async_connect(peer, std::forward<ConnectHandler>(handler));
  }

  template <typename HandshakeHandler>
  auto async_handshake(boost::asio::ssl::stream_base::handshake_type t, HandshakeHandler&& handler)
  {
    return stream_.async_handshake(t, std::forward<HandshakeHandler>(handler));
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

template <typename T> struct IsTlsStream<TlsStream<T>> : public std::true_type {
};

#endif // ENABLE_TLS

#ifdef BUILD_TEST

class TestSocket {
private:
  using Buffer = boost::beast::flat_buffer;

  template <typename OutputIt> static auto read(Buffer& buf, OutputIt it, size_t n)
  {
    assertTrue(buf.size() > 0);
    auto copied = std::min(buf.size(), n);
    std::copy_n(boost::asio::buffers_begin(buf.data()), copied, it);
    buf.consume(copied);
    return copied;
  }

  template <typename InputIt> static auto write(Buffer& buf, InputIt it, size_t n)
  {
    std::copy_n(it, n, boost::asio::buffers_begin(buf.prepare(n)));
    buf.commit(n);
    return n;
  }

public:
  template <typename MutableBufferSequence> auto read(MutableBufferSequence const& buf)
  {
    return read(rBuf_, boost::asio::buffers_begin(buf), boost::asio::buffer_size(buf));
  }

  template <typename ConstBufferSequence> auto write(ConstBufferSequence const& buf)
  {
    return write(wBuf_, boost::asio::buffers_begin(buf), boost::asio::buffer_size(buf));
  }

  auto fill(std::initializer_list<uint8_t> l) { return write(rBuf_, std::cbegin(l), l.size()); }

  auto fill(ConstBuffer<uint8_t> buf) { return write(rBuf_, std::cbegin(buf), buf.size()); }

  auto flush(MutableBuffer<uint8_t> buf) { return read(wBuf_, std::cbegin(buf), buf.size()); }

  auto available() const { return wBuf_.size(); }

private:
  Buffer rBuf_;
  Buffer wBuf_;
};

/*
 * Faked stream class used for unit test. It's supposed to satisfy the following concepts at least:
 *   - SyncReadStream: \
 *       https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/SyncReadStream.html
 *   - SyncWriteStream: \
 *       https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/SyncWriteStream.html
 * w
 */
class TestStream {
private:
  using ErrorCode = boost::system::error_code;

public:
  explicit TestStream(TestSocket& socket, bool open = false) : socket_{socket}, open_{open} {}

  bool is_open() const { return open_; }

  void close(ErrorCode& ec)
  {
    ec = {};
    open_ = false;
  }

  void connect() { open_ = true; }

  template <typename MutableBufferSequence> auto read_some(MutableBufferSequence const& buf)
  {
    assertTrue(is_open());
    return socket_.read(buf);
  }

  template <typename MutableBufferSequence>
  auto read_some(MutableBufferSequence const& buf, ErrorCode& ec)
  {
    ec = {};
    return read_some(buf);
  }

  template <typename ConstBufferSequence> auto write_some(ConstBufferSequence const& buf)
  {
    assertTrue(is_open());
    return socket_.write(buf);
  }

  template <typename ConstBufferSequence>
  auto write_some(ConstBufferSequence const& buf, ErrorCode& ec)
  {
    ec = {};
    return write_some(buf);
  }

private:
  TestSocket& socket_;
  bool open_;
};

#endif // BUILD_TEST

/*
 *  boost::asio::async_connect only supports basic_socket, so asyncConnect is intended to
 *  support all the stream classes with async_connect member function template.
 */
template <typename Stream, typename ConnectHandler>
auto asyncConnect(Stream& stream, ResolveResults results, ConnectHandler&& handler)
{
  // FIXME the life term of results should be extended until all of the async-ops are accomplished,
  //   but according to the implementation of ResolveResults::iterator, the iterator will
  //   fulfill the extension. So, keep it this way here.
  return detail::asyncInitiate<void(boost::system::error_code)>(
      [](auto&& h, auto& stream, auto results) {
        using Handler = decltype(h);
        static_assert(std::is_invocable_v<Handler, boost::system::error_code const&>);
        if (results.empty()) {
          boost::asio::post(boost::asio::get_associated_executor(h),
                            [h = std::forward<Handler>(h)]() mutable {
                              std::invoke(h, boost::asio::error::host_not_found);
                            });
          return;
        }
        auto first = std::cbegin(results);
        stream.async_connect(*first, detail::IteratorConnectOperator{stream, first, h});
      },
      std::forward<ConnectHandler>(handler), stream, std::move(results));
}

} // namespace pichi::net

namespace boost::asio {

template <typename Stream, typename Iterator, typename ConnectHandler>
bool asio_handler_is_continuation(
    pichi::net::detail::IteratorConnectOperator<Stream, Iterator, ConnectHandler>*)
{
  return true;
}

template <typename F, typename Stream, typename Iterator, typename ConnectHandler>
void asio_handler_invoke(
    F&& f, pichi::net::detail::IteratorConnectOperator<Stream, Iterator, ConnectHandler>*)
{
  std::invoke(std::forward<F>(f));
}

template <typename Stream, typename Iterator, typename ConnectHandler>
struct associated_executor<
    pichi::net::detail::IteratorConnectOperator<Stream, Iterator, ConnectHandler>> {
  using type = executor;

  static type
  get(pichi::net::detail::IteratorConnectOperator<Stream, Iterator, ConnectHandler> const& h,
      executor const& = executor{}) BOOST_ASIO_NOEXCEPT
  {
    return get_associated_executor(h.h_);
  }
};

} // namespace boost::asio

#endif // PICHI_NET_STREAM_HPP
