#include <pichi/config.hpp>
// Include config.hpp first
#include <boost/asio/ip/tcp.hpp>
#include <cassert>
#include <pichi/net/asio.hpp>
#include <pichi/net/tunnel.hpp>
#include <vector>

using namespace std;

namespace pichi::net {

template <typename Iterator, typename Socket> TunnelIngress<Iterator, Socket>::~TunnelIngress()
{
  if (!released_) {
    balancer_.release(it_);
    released_ = true;
  }
}

template <typename Iterator, typename Socket>
size_t TunnelIngress<Iterator, Socket>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return pichi::net::readSome(socket_, buf, yield);
}

template <typename Iterator, typename Socket>
void TunnelIngress<Iterator, Socket>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  pichi::net::write(socket_, buf, yield);
}

template <typename Iterator, typename Socket>
void TunnelIngress<Iterator, Socket>::close(Yield yield)
{
  pichi::net::close(socket_, yield);
  if (!released_) {
    balancer_.release(it_);
    released_ = true;
  }
}

template <typename Iterator, typename Socket> bool TunnelIngress<Iterator, Socket>::readable() const
{
  return isOpen(socket_);
}

template <typename Iterator, typename Socket> bool TunnelIngress<Iterator, Socket>::writable() const
{
  return isOpen(socket_);
}

template <typename Iterator, typename Socket>
Endpoint TunnelIngress<Iterator, Socket>::readRemote(Yield)
{
  it_ = balancer_.select();
  return *it_;
}

template <typename Iterator, typename Socket> void TunnelIngress<Iterator, Socket>::confirm(Yield)
{
}

template class TunnelIngress<vector<Endpoint>::const_iterator, boost::asio::ip::tcp::socket>;

} // namespace pichi::net
