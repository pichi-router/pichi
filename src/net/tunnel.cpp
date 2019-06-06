#include <boost/asio/ip/tcp.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/tunnel.hpp>

namespace pichi::net {

template <typename Socket> TunnelIngress<Socket>::~TunnelIngress() = default;

template <typename Socket>
size_t TunnelIngress<Socket>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return pichi::net::readSome(socket_, buf, yield);
}

template <typename Socket> void TunnelIngress<Socket>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  pichi::net::write(socket_, buf, yield);
}

template <typename Socket> void TunnelIngress<Socket>::close(Yield yield)
{
  pichi::net::close(socket_, yield);
}

template <typename Socket> bool TunnelIngress<Socket>::readable() const { return isOpen(socket_); }

template <typename Socket> bool TunnelIngress<Socket>::writable() const { return isOpen(socket_); }

template <typename Socket> Endpoint TunnelIngress<Socket>::readRemote(Yield) { return endpoint_; }

template <typename Socket> void TunnelIngress<Socket>::confirm(Yield) {}

template <typename Socket> void TunnelIngress<Socket>::disconnect(Yield) {}

template class TunnelIngress<boost::asio::ip::tcp::socket>;

} // namespace pichi::net
