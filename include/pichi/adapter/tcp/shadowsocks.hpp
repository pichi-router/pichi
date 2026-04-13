#ifndef PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP
#define PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP

#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/stream/shadowsocks.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>

namespace pichi::adapter::tcp {

template <typename Socket> class Shadowsocks {
public:
  explicit Shadowsocks(vo::Ingress const&, Socket);

  explicit Shadowsocks(vo::Egress const&, IOExecutor const&);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);

  Awaitable<void> close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();

  Awaitable<void> connect(Endpoint const&);
  Awaitable<void> disconnect(boost::system::error_code const&);

private:
  stream::Shadowsocks<Socket> stream_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP
