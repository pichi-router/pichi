#ifndef PICHI_NET_ASIO_HPP
#define PICHI_NET_ASIO_HPP

#include <boost/asio/buffer.hpp>
#include <memory>
#include <pichi/api/ingress_manager.hpp>
#include <pichi/buffer.hpp>
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

class io_context;

namespace ssl {

template <typename Stream> class stream;

} // namespace ssl

} // namespace boost::asio

namespace pichi::api {

struct IngressVO;
struct EgressVO;

} // namespace pichi::api

namespace pichi::net {

struct Endpoint;
struct Ingress;
struct Egress;

template <typename T> struct IsSslStream : public std::false_type {
};

template <typename T> struct IsSslStream<boost::asio::ssl::stream<T>> : public std::true_type {
};

template <typename T> inline constexpr bool IsSslStreamV = IsSslStream<T>::value;

template <typename Socket, typename Yield> void connect(Endpoint const&, Socket&, Yield);
template <typename Socket, typename Yield> void read(Socket&, MutableBuffer<uint8_t>, Yield);
template <typename Socket, typename Yield> size_t readSome(Socket&, MutableBuffer<uint8_t>, Yield);
template <typename Socket, typename Yield> void write(Socket&, ConstBuffer<uint8_t>, Yield);
template <typename Socket, typename Yield> void close(Socket&, Yield);
template <typename Socket> bool isOpen(Socket const&);

template <typename Socket> std::unique_ptr<Ingress> makeIngress(api::IngressHolder&, Socket&&);
std::unique_ptr<Egress> makeEgress(api::EgressVO const&, boost::asio::io_context&);

} // namespace pichi::net

#endif // PICHI_NET_ASIO_HPP
