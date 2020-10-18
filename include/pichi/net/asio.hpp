#ifndef PICHI_NET_ASIO_HPP
#define PICHI_NET_ASIO_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/write.hpp>
#include <pichi/common/adapter.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/net/stream.hpp>
#include <type_traits>

namespace boost::asio {

template <typename PodType> inline mutable_buffer buffer(pichi::MutableBuffer<PodType> origin)
{
  return {origin.data(), origin.size() * sizeof(PodType)};
}

template <typename PodType>
inline mutable_buffer buffer(pichi::MutableBuffer<PodType> origin, size_t size)
{
  assert(size <= origin.size());
  return {origin.data(), size * sizeof(PodType)};
}

template <typename PodType> inline const_buffer buffer(pichi::ConstBuffer<PodType> origin)
{
  return {origin.data(), origin.size() * sizeof(PodType)};
}

template <typename PodType>
inline const_buffer buffer(pichi::ConstBuffer<PodType> origin, size_t size)
{
  assert(size <= origin.size());
  return {origin.data(), size * sizeof(PodType)};
}

}  // namespace boost::asio

namespace pichi::net {

template <typename Stream, typename Results, typename Yield>
void connect(Results next, Stream& stream, Yield yield)
{
#ifdef BUILD_TEST
  if constexpr (std::is_same_v<Stream, TestStream>) {
    suppressC4100(next);
    suppressC4100(yield);
    stream.connect();
    return;
  }
  else {
#endif  // BUILD_TEST
    asyncConnect(stream, next, yield);
#ifdef BUILD_TEST
  }
#endif  // BUILD_TEST
  if constexpr (IsTlsStreamV<Stream>) {
    stream.async_handshake(boost::asio::ssl::stream_base::client, yield);
  }
}

template <typename Stream, typename Yield>
void read(Stream& stream, MutableBuffer<uint8_t> buf, Yield yield)
{
#ifdef BUILD_TEST
  if constexpr (std::is_same_v<Stream, TestStream>)
    boost::asio::read(stream, boost::asio::buffer(buf));
  else
#endif  // BUILD_TEST
    boost::asio::async_read(stream, boost::asio::buffer(buf), yield);
}

template <typename Stream, typename Yield>
size_t readSome(Stream& stream, MutableBuffer<uint8_t> buf, Yield yield)
{
#ifdef BUILD_TEST
  if constexpr (std::is_same_v<Stream, TestStream>) {
    return stream.read_some(boost::asio::buffer(buf));
  }
  else
#endif  // BUILD_TEST
    return stream.async_read_some(boost::asio::buffer(buf), yield);
}

template <typename Stream, typename Yield>
void write(Stream& stream, ConstBuffer<uint8_t> buf, Yield yield)
{
#ifdef BUILD_TEST
  if constexpr (std::is_same_v<Stream, TestStream>)
    boost::asio::write(stream, boost::asio::buffer(buf));
  else
#endif  // BUILD_TEST
    boost::asio::async_write(stream, boost::asio::buffer(buf), yield);
}

template <typename Stream, typename Yield> void close(Stream& stream, Yield yield)
{
  // This function is supposed to be 'noexcept' because it's always invoked in
  // the desturctors.
  // TODO log it
  auto ec = boost::system::error_code{};
  if constexpr (IsTlsStreamV<Stream>) {
    stream.async_shutdown(yield[ec]);
    stream.close(ec);
  }
  else {
    suppressC4100(yield);
    stream.close(ec);
  }
}

}  // namespace pichi::net

#endif  // PICHI_NET_ASIO_HPP
