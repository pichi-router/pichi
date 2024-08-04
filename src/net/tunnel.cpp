#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/ip/tcp.hpp>
#include <pichi/net/helper.hpp>
#include <pichi/net/tunnel.hpp>

using namespace std;

namespace pichi::net {

template <typename Socket> TunnelIngress<Socket>::~TunnelIngress()
{
  if (!released_) {
    pBalancer_->release(it_);
    released_ = true;
  }
}

template <typename Socket> size_t TunnelIngress<Socket>::recv(MutableBuffer buf, Yield yield)
{
  return pichi::net::readSome(socket_, buf, yield);
}

template <typename Socket> void TunnelIngress<Socket>::send(ConstBuffer buf, Yield yield)
{
  pichi::net::write(socket_, buf, yield);
}

template <typename Socket> void TunnelIngress<Socket>::close(Yield yield)
{
  pichi::net::close(socket_, yield);
  if (!released_) {
    pBalancer_->release(it_);
    released_ = true;
  }
}

template <typename Socket> bool TunnelIngress<Socket>::readable() const
{
  return socket_.is_open();
}

template <typename Socket> bool TunnelIngress<Socket>::writable() const
{
  return socket_.is_open();
}

template <typename Socket> Endpoint TunnelIngress<Socket>::readRemote(Yield)
{
  it_ = pBalancer_->select();
  return *it_;
}

template <typename Socket> void TunnelIngress<Socket>::confirm(Yield) {}

template class TunnelIngress<boost::asio::ip::tcp::socket>;

}  // namespace pichi::net
