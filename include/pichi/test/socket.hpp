#ifndef PICHI_TEST_SOCKET_HPP
#define PICHI_TEST_SOCKET_HPP

#include "config.h"

#ifdef BUILD_TEST

#include <boost/asio/async_result.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <functional>
#include <pichi/asserts.hpp>

namespace pichi::test {

extern boost::asio::io_context io;

/*
 * Faked socket class used for unit test. It's supposed to satisfy the following concepts at least:
 *   - AsyncReadStream: \
 *       https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/AsyncReadStream.html
 *   - AsyncWriteStream: \
 *       https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/AsyncWriteStream.html
 * w
 */
class Socket {
private:
  using ErrorCode = boost::system::error_code;
  template <typename Handler>
  using AsyncCompletion = boost::asio::async_completion<Handler, void(ErrorCode, size_t)>;

public:
  template <typename... Args> Socket(Args&&...) {}

  bool is_open() const { return open_; }

  void close(ErrorCode& ec)
  {
    ec = {};
    open_ = false;
  }

  void connect() { open_ = true; }

  template <typename MutableBufferSequence, typename ReadHandler>
  auto async_read_some(MutableBufferSequence const& buf, ReadHandler&& h)
  {
    using boost::asio::buffer_copy;
    assertTrue(is_open());
    auto init = AsyncCompletion<ReadHandler>{h};
    std::invoke(init.completion_handler, ErrorCode{}, buffer_copy(buf, buffer_.data()));
    return init.result.get();
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  auto async_write_some(ConstBufferSequence const& buf, WriteHandler&& h)
  {
    assertTrue(is_open());
    auto copied = boost::asio::buffer_size(buf);
    boost::asio::buffer_copy(buffer_.prepare(copied), buf);
    buffer_.commit(copied);
    auto init = AsyncCompletion<WriteHandler>{h};
    std::invoke(init.completion_handler, ErrorCode{}, copied);
    return init.result.get();
  }

  auto get_executor() { return io.get_executor(); }

private:
  boost::beast::flat_buffer buffer_;
  bool open_ = false;
};

} // namespace pichi::test

#endif // BUILD_TEST

#endif // PICHI_TEST_SOCKET_HPP
