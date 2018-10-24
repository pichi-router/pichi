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
  auto guard = makeScopeGuard([=]() {
    from->close();
    to->close();
  });
  auto buf = array<uint8_t, net::MAX_FRAME_SIZE>{};
  while (from->readable() && to->writable()) to->send({buf, from->recv(buf, yield)}, yield);
  guard.disable();
}

Session::Session(asio::io_context& io, Session::IngressPtr&& ingress, Session::EgressPtr&& egress)
  : strand_{io}, ingress_{move(ingress)}, egress_{move(egress)}
{
}

void Session::start(net::Endpoint const& remote, net::Endpoint const& next)
{
  spawn([=, self = shared_from_this()](auto yield) {
    auto i = ingress_.get();
    auto e = egress_.get();
    e->connect(remote, next, yield);
    i->confirm(yield);
    spawn([f = i, t = e, self = self](auto yield) { bridge(f, t, yield); });
    spawn([f = e, t = i, self = self](auto yield) { bridge(f, t, yield); });
  });
}

template <typename Function> void Session::spawn(Function&& func)
{
  static_assert(is_invocable_v<Function, asio::yield_context>);
  asio::spawn(strand_, [f = forward<Function>(func)](auto yield) mutable {
    try {
      f(yield);
    }
    catch (Exception& e) {
      cout << "Pichi Error: " << e.what() << endl;
    }
    catch (sys::system_error& e) {
      if (e.code() == asio::error::eof || e.code() == asio::error::operation_aborted) return;
      cout << "Socket Error: " << e.what() << endl;
    }
  });
}

} // namespace pichi::api
