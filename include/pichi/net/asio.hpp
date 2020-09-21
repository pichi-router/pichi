#ifndef PICHI_NET_ASIO_HPP
#define PICHI_NET_ASIO_HPP

#include <boost/asio/buffer.hpp>
#include <memory>
#include <pichi/common/adapter.hpp>
#include <pichi/common/buffer.hpp>

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

class io_context;

}  // namespace boost::asio

namespace pichi::vo {

struct Ingress;
struct Egress;

}  // namespace pichi::vo

namespace pichi::api {

struct IngressHolder;

}  // namespace pichi::api

namespace pichi::net {

template <typename Socket, typename Yield> void connect(ResolveResults, Socket&, Yield);
template <typename Socket, typename Yield> void read(Socket&, MutableBuffer<uint8_t>, Yield);
template <typename Socket, typename Yield> size_t readSome(Socket&, MutableBuffer<uint8_t>, Yield);
template <typename Socket, typename Yield> void write(Socket&, ConstBuffer<uint8_t>, Yield);
template <typename Socket, typename Yield> void close(Socket&, Yield);

template <typename Socket> std::unique_ptr<Ingress> makeIngress(api::IngressHolder&, Socket&&);
std::unique_ptr<Egress> makeEgress(vo::Egress const&, boost::asio::io_context&);

}  // namespace pichi::net

#endif  // PICHI_NET_ASIO_HPP
