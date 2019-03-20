#ifndef PICHI_TEST_SOCKET_HPP
#define PICHI_TEST_SOCKET_HPP

#include "config.h"

#ifdef BUILD_TEST

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <pichi/asserts.hpp>

namespace pichi::test {

/*
 * Faked socket class used for unit test. It's supposed to satisfy the following concepts at least:
 *   - SyncReadStream: \
 *       https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/SyncReadStream.html
 *   - SyncWriteStream: \
 *       https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/SyncWriteStream.html
 * w
 */
class Socket {
private:
  using ErrorCode = boost::system::error_code;

public:
  template <typename... Args> explicit Socket(Args&&...) {}

  explicit Socket(bool open) : open_{open} {}

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
    assertTrue(boost::asio::buffer_size(buf) <= buffer_.size());
    auto copied = boost::asio::buffer_copy(buf, buffer_.data());
    buffer_.consume(copied);
    return copied;
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
    auto copied = boost::asio::buffer_size(buf);
    boost::asio::buffer_copy(buffer_.prepare(copied), buf);
    buffer_.commit(copied);
    return copied;
  }

  template <typename ConstBufferSequence>
  auto write_some(ConstBufferSequence const& buf, ErrorCode& ec)
  {
    ec = {};
    return write_some(buf);
  }

private:
  boost::beast::flat_buffer buffer_;
  bool open_ = false;
};

} // namespace pichi::test

#endif // BUILD_TEST

#endif // PICHI_TEST_SOCKET_HPP
