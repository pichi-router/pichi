#ifndef PICHI_NET_HELPER_HPP
#define PICHI_NET_HELPER_HPP

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/traits.hpp>
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

template <typename Stream, typename Yield> void handshake(Stream& stream, Yield yield)
{
  static_assert(stream::IsAsyncStream<Stream>);
  static_assert(!stream::IsRawStream<Stream>);
  if constexpr (!stream::IsRawStream<typename Stream::next_layer_type>)
    handshake(stream.next_layer(), yield);
  stream.async_handshake(yield);
}

template <typename Stream, typename Results, typename Yield>
void connect(Results next, Stream& stream, Yield yield)
{
  suppressC4100(next, yield);
  if constexpr (stream::IsAsyncStream<Stream>)
    boost::asio::async_connect(boost::beast::get_lowest_layer(stream), next, yield);
  else
    stream.connect();
  if constexpr (!stream::IsRawStream<Stream>) {
    handshake(stream, yield);
  }
}

template <typename Stream, typename Yield> void accept(Stream& stream, Yield yield)
{
  suppressC4100(stream, yield);
  if constexpr (!stream::IsRawStream<Stream>) {
    static_assert(stream::IsAsyncStream<Stream>);
    if constexpr (!stream::IsRawStream<typename Stream::next_layer_type>) {
      accept(stream.next_layer(), yield);
    }
    stream.async_accept(yield);
  }
}

template <typename Stream, typename Yield>
void read(Stream& stream, MutableBuffer<uint8_t> buf, Yield yield)
{
  suppressC4100(yield);
  if constexpr (stream::IsAsyncStream<Stream>)
    boost::asio::async_read(stream, boost::asio::buffer(buf), yield);
  else
    boost::asio::read(stream, boost::asio::buffer(buf));
}

template <typename Stream, typename Yield>
size_t readSome(Stream& stream, MutableBuffer<uint8_t> buf, Yield yield)
{
  suppressC4100(yield);
  if constexpr (stream::IsAsyncStream<Stream>)
    return stream.async_read_some(boost::asio::buffer(buf), yield);
  else
    return stream.read_some(boost::asio::buffer(buf));
}

template <typename Stream, typename Parser, typename DynamicBuffer, typename Yield>
size_t readHttpHeader(Stream& stream, DynamicBuffer& buf, Parser& parser, Yield yield)
{
  suppressC4100(yield);
  if constexpr (stream::IsAsyncStream<Stream>)
    return boost::beast::http::async_read_header(stream, buf, parser, yield);
  else
    return boost::beast::http::read_header(stream, buf, parser);
}

template <typename Stream, typename Yield>
void write(Stream& stream, ConstBuffer<uint8_t> buf, Yield yield)
{
  suppressC4100(yield);
  if constexpr (stream::IsAsyncStream<Stream>)
    boost::asio::async_write(stream, boost::asio::buffer(buf), yield);
  else
    boost::asio::write(stream, boost::asio::buffer(buf));
}

template <typename Stream, typename Message, typename WriteHandler>
void writeHttp(Stream& stream, Message& msg, WriteHandler&& h)
{
  suppressC4100(h);
  if constexpr (stream::IsAsyncStream<Stream>)
    boost::beast::http::async_write(stream, msg, std::forward<WriteHandler>(h));
  else
    boost::beast::http::write(stream, msg);
}

template <typename Stream, typename Serializer, typename Yield>
void writeHttpHeader(Stream& stream, Serializer&& sr, Yield yield)
{
  suppressC4100(yield);
  if constexpr (stream::IsAsyncStream<Stream>)
    boost::beast::http::async_write_header(stream, sr, yield);
  else
    boost::beast::http::write_header(stream, sr);
}

template <typename Stream, typename Yield> void close(Stream& stream, Yield yield)
{
  // This function is supposed to be 'noexcept' because it's always invoked in
  // the desturctors.

  suppressC4100(yield);

  // TODO log the errors
  auto ec = boost::system::error_code{};

  /*
   *  TODO abort() will be invoked here in Windows even though no exception
   *       is explicitly thrown. The root cause is unknown for now.
   */
#ifndef _MSC_VER
  if constexpr (!stream::IsRawStream<Stream>) stream.async_shutdown(yield[ec]);
#endif  // _MSC_VER

  boost::beast::get_lowest_layer(stream).close(ec);
}

}  // namespace pichi::net

#endif  // PICHI_NET_HELPER_HPP
