#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <pichi/api/session.hpp>
#include <pichi/common/exception.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>
#include <pichi/net/spawn.hpp>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;

using asio::ip::tcp;

namespace pichi::api {

static void bridge(net::Adapter& from, net::Adapter& to, asio::yield_context yield)
{
  auto buf = array<uint8_t, net::MAX_FRAME_SIZE>{};
  while (from.readable() && to.writable()) to.send({buf, from.recv(buf, yield)}, yield);
}

Session::~Session() = default;

Session::Session(asio::io_context& io, Session::IngressPtr&& ingress, Session::EgressPtr&& egress)
  : strand_{io}, ingress_{move(ingress)}, egress_{move(egress)}
{
}

void Session::start(net::Endpoint const& remote, net::Endpoint const& next)
{
  // FIXME Not copying for self because net::spawn ensures that
  //   Function and ExceptionHandler share the same scope.
  net::spawn(
      strand_,
      [=, self = shared_from_this()](auto yield) {
        auto resolver = tcp::resolver{strand_.context()};
        egress_->connect(remote, resolver.async_resolve(next.host_, next.port_, yield), yield);
        ingress_->confirm(yield);
        net::spawn(
            strand_, [self, this](auto yield) { bridge(*ingress_, *egress_, yield); },
            [this](auto, auto yield) noexcept { this->close(yield); });
        net::spawn(
            strand_, [self, this](auto yield) { bridge(*egress_, *ingress_, yield); },
            [this](auto, auto yield) noexcept { this->close(yield); });
      },
      [this](auto eptr, auto yield) noexcept { ingress_->disconnect(eptr, yield); });
}

void Session::start(net::Endpoint const& peer) { start(peer, peer); }

template <typename Yield> void Session::close(Yield yield)
{
  ingress_->close(yield);
  egress_->close(yield);
}

} // namespace pichi::api
