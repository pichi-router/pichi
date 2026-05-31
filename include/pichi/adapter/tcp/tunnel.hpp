#ifndef PICHI_ADAPTER_TCP_TUNNEL_HPP
#define PICHI_ADAPTER_TCP_TUNNEL_HPP

#include <boost/asio/ip/tcp.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/service/balancer.hpp>
#include <pichi/vo/ingress.hpp>
#include <string>

namespace pichi::adapter::tcp {

class Tunnel {
private:
  using Balancer = service::BalancerPtr;
  using Socket   = boost::asio::ip::tcp::socket;

public:
  explicit Tunnel(vo::Ingress const&, Socket);
  ~Tunnel();

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();
  Awaitable<void>     disconnect(boost::system::error_code const&);

private:
  Balancer    balancer_;
  std::string name_;
  Endpoint    peer_;
  Socket      socket_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_TUNNEL_HPP
