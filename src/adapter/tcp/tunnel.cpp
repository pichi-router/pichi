#include "pichi/common/config.hpp"
#include <pichi/adapter/tcp/tunnel.hpp>
#include <pichi/stream/helpers.hpp>

namespace sys = boost::system;

namespace pichi::adapter::tcp {

Tunnel::Tunnel(vo::Ingress const& vo, Socket s)
  : balancer_{nullptr}, name_{vo.name_}, peer_{}, socket_{std::move(s)}
{
}

Tunnel::~Tunnel()
{
  if (balancer_ != nullptr) balancer_->release(peer_);
}

Awaitable<size_t> Tunnel::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(socket_, buf);
}

Awaitable<void> Tunnel::send(ConstBuffer buf) { co_await stream::write(socket_, buf); }

Awaitable<void> Tunnel::close() { co_await stream::close(socket_); }

Awaitable<Endpoint> Tunnel::read_remote()
{
  balancer_ = co_await service::get_balancer(socket_.get_executor(), name_);
  peer_     = co_await balancer_->select();
  co_return peer_;
}

Awaitable<void> Tunnel::confirm() { co_return; }

Awaitable<void> Tunnel::disconnect(sys::error_code const&) { co_return; }

}  // namespace pichi::adapter::tcp
