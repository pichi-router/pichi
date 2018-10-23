#include <array>
#include <boost/asio/spawn2.hpp>
#include <iostream>
#include <pichi/api/session.hpp>
#include <pichi/exception.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>
#include <pichi/scope_guard.hpp>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;

namespace pichi::api {

static void bridge(net::Adapter* from, net::Adapter* to, asio::yield_context yield)
{
  try {
    auto guard = makeScopeGuard([from, to]() {
      from->close();
      to->close();
    });
    auto buf = array<uint8_t, net::MAX_FRAME_SIZE>{};
    while (from->readable() && to->writable()) to->send({buf, from->recv(buf, yield)}, yield);
    guard.disable();
  }
  catch (Exception& e) {
    cout << "Pichi Error: " << e.what() << endl;
  }
  catch (sys::system_error& e) {
    if (e.code() == asio::error::eof || e.code() == asio::error::operation_aborted) return;
    cout << "Socket Error: " << e.what() << endl;
  }
}

Session::Session(asio::io_context& io, Session::IngressPtr&& ingress, Session::EgressPtr&& egress)
  : strand_{io}, ingress_{move(ingress)}, egress_{move(egress)}
{
}

void Session::start(net::Endpoint const& remote, net::Endpoint const& next)
{
  asio::spawn(strand_, [=, self = shared_from_this()](auto yield) {
    egress_->connect(remote, next, yield);
    ingress_->confirm(yield);
    asio::spawn(strand_, [this, self = shared_from_this()](auto yield) {
      bridge(ingress_.get(), egress_.get(), yield);
    });
    asio::spawn(strand_, [this, self = shared_from_this()](auto yield) {
      bridge(egress_.get(), ingress_.get(), yield);
    });
  });
}

} // namespace pichi::api
