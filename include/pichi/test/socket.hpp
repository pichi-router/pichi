#ifndef PICHI_TEST_SOCKET_HPP
#define PICHI_TEST_SOCKET_HPP

#include <pichi/config.hpp>

#ifdef BUILD_TEST

#include <algorithm>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/system/error_code.hpp>
#include <initializer_list>
#include <pichi/asserts.hpp>

namespace pichi::test {

class Socket {
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
class Stream {
private:
  using ErrorCode = boost::system::error_code;

public:
  explicit Stream(Socket& socket, bool open = false) : socket_{socket}, open_{open} {}

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
  Socket& socket_;
  bool open_;
};

} // namespace pichi::test

#endif // BUILD_TEST

#endif // PICHI_TEST_SOCKET_HPP
