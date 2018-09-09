#ifndef PICHI_NET_ASIO_HPP
#define PICHI_NET_ASIO_HPP

#include <boost/asio/buffer.hpp>
#include <memory>
#include <pichi/buffer.hpp>

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

} // namespace boost::asio

namespace pichi::api {

class InboundVO;
class OutboundVO;

} // namespace pichi::api

namespace pichi::net {

class Endpoint;
class Inbound;
class Outbound;

template <typename Socket, typename Yield> void connect(Endpoint const&, Socket&, Yield);
template <typename Socket, typename Yield> void read(Socket&, MutableBuffer<uint8_t>, Yield);
template <typename Socket, typename Yield> void write(Socket&, ConstBuffer<uint8_t>, Yield);

template <typename Socket> std::unique_ptr<Inbound> makeInbound(api::InboundVO const&, Socket&&);
template <typename Socket> std::unique_ptr<Outbound> makeOutbound(api::OutboundVO const&, Socket&&);

} // namespace pichi::net

#endif // PICHI_NET_ASIO_HPP
