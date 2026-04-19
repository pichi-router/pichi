#ifndef PICHI_STREAM_TEST_HPP
#define PICHI_STREAM_TEST_HPP

#include <algorithm>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <deque>
#include <pichi/common/asserts.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/stream/helpers.hpp>
#include <pichi/stream/traits.hpp>

namespace pichi::stream {

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
    assertFalse(rBufs_.empty());
    auto ret = read(rBufs_.front(), boost::asio::buffers_begin(buf), boost::asio::buffer_size(buf));
    if (rBufs_.front().size() == 0) rBufs_.pop_front();
    return ret;
  }

  template <typename ConstBufferSequence> auto write(ConstBufferSequence const& buf)
  {
    return write(wBuf_, boost::asio::buffers_begin(buf), boost::asio::buffer_size(buf));
  }

  auto fill(std::initializer_list<uint8_t> l)
  {
    rBufs_.emplace_back();
    return write(rBufs_.back(), std::cbegin(l), l.size());
  }

  auto fill(ConstBuffer buf)
  {
    rBufs_.emplace_back();
    return write(rBufs_.back(), std::cbegin(buf), buf.size());
  }

  auto flush(MutableBuffer buf) { return read(wBuf_, std::cbegin(buf), buf.size()); }

  auto available() const { return wBuf_.size(); }

private:
  std::deque<Buffer> rBufs_;
  Buffer             wBuf_;
};

/*
 * Faked stream class used for unit test. It's supposed to satisfy the following
 * concepts at least:
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
    ec    = {};
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
  bool        open_;
};

template <> struct AsyncStream<TestStream> : public std::false_type {};

}  // namespace pichi::stream

namespace pichi::unit_test {

class TestSocket {
private:
  using ErrorCode = boost::system::error_code;

public:
  using endpoint_type = boost::asio::ip::tcp::endpoint;
  using executor_type = boost::asio::readable_pipe::executor_type;

  using lowest_layer_type = TestSocket;

  explicit TestSocket(IOExecutor const&);

  executor_type get_executor();

  bool is_open() const;
  void close();

  lowest_layer_type& lowest_layer() { return *this; }

  template <typename MutableBufferSequence, typename ReadToken>
  auto async_read_some(MutableBufferSequence const& buf, ReadToken&& token)
  {
    return in_.async_read_some(buf, std::forward<ReadToken>(token));
  }

  template <typename ConstBufferSequence, typename WriteToken>
  auto async_write_some(ConstBufferSequence const& buf, WriteToken&& token)
  {
    return out_.async_write_some(buf, std::forward<WriteToken>(token));
  }

  template <typename ConnectToken>
  auto async_connect(boost::asio::ip::tcp::endpoint const&, ConnectToken&& token)
  {
    return stream::async_initiate<void(ErrorCode)>(
        get_executor(),
        std::forward<ConnectToken>(token),
        []() -> Awaitable<void> { co_return; }  // Do nothing
    );
  }

  TestSocket peer();

private:
  boost::asio::readable_pipe in_;
  boost::asio::writable_pipe out_;
};

}  // namespace pichi::unit_test

#endif  // PICHI_STREAM_TEST_HPP
