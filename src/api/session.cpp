#include <array>
#include <iostream>
#include <pichi/api/session.hpp>
#include <pichi/exception.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>
#include <pichi/net/spawn.hpp>
#include <pichi/scope_guard.hpp>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;

namespace pichi::api {

static void bridge(net::Adapter* from, net::Adapter* to, asio::yield_context yield)
{
  auto guard = makeScopeGuard([=]() {
    from->close();
    to->close();
  });
  auto buf = array<uint8_t, net::MAX_FRAME_SIZE>{};
  while (from->readable() && to->writable()) to->send({buf, from->recv(buf, yield)}, yield);
  guard.disable();
}

Session::~Session() = default;

Session::Session(asio::io_context& io, Session::IngressPtr&& ingress, Session::EgressPtr&& egress)
  : strand_{io}, ingress_{move(ingress)}, egress_{move(egress)}
{
}

void Session::start(net::Endpoint const& remote, net::Endpoint const& next)
{
  net::spawn(strand_, [=, self = shared_from_this()](auto yield) {
    auto i = ingress_.get();
    auto e = egress_.get();
    e->connect(remote, next, yield);
    i->confirm(yield);
    net::spawn(strand_, [f = i, t = e, self](auto yield) { bridge(f, t, yield); });
    net::spawn(strand_, [f = e, t = i, self](auto yield) { bridge(f, t, yield); });
  });
}

} // namespace pichi::api
