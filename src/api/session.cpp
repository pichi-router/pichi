#include <array>
#include <iostream>
#include <pichi/api/session.hpp>
#include <pichi/exception.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>
#include <pichi/net/spawn.hpp>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;

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
  net::spawn(strand_, [=, self = shared_from_this()](auto yield) {
    egress_->connect(remote, next, yield);
    ingress_->confirm(yield);
    net::spawn(
        strand_, [self, this](auto yield) { bridge(*ingress_, *egress_, yield); },
        // FIXME Not copying for self because net::spawn ensures that
        //   Function and ExceptionHandler share the same scope.
        [this](auto, auto) noexcept { close(); });
    net::spawn(
        strand_, [self, this](auto yield) { bridge(*egress_, *ingress_, yield); },
        [this](auto, auto) noexcept { close(); });
  });
}

void Session::close()
{
  ingress_->close();
  egress_->close();
}

} // namespace pichi::api
