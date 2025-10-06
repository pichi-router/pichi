#include "pichi/common/config.hpp"
#include <boost/asio/deferred.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <chrono>
#include <format>
#include <iostream>
#include <pichi/actor/session.hpp>
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/crypto/key.hpp>
#include <pichi/stream/helpers.hpp>

namespace asio = boost::asio;
namespace sys  = boost::system;

namespace pichi::actor {

template <typename From, typename To> Awaitable<void> bridge(From& from, To& to)
{
  auto ec = sys::error_code{};
  while (true) {
    auto buf = std::array<uint8_t, 0xffff>{};
    auto len =
        co_await redirect(std::visit([&buf](auto&& from) { return from.recv(buf); }, from), ec);
    if (ec) break;
    co_await redirect(std::visit([&buf, len](auto&& to) { return to.send({buf, *len}); }, to), ec);
    if (ec) break;
  }
  co_await std::visit([](auto&& a) { return a.close(); }, from);
  co_await std::visit([](auto&& a) { return a.close(); }, to);
  asio::detail::throw_error(ec);
}

Awaitable<adapter::tcp::Egress>
    Session::handshake(adapter::tcp::Ingress& ingress, AdapterType itype)
{
  auto peer = co_await std::visit([](auto&& ingress) { return ingress.read_remote(); }, ingress);

  auto [rname, ename, evo] = co_await router_->route(peer, iname_, itype);

  auto egress = adapter::tcp::create_egress(evo, ex_);

  co_await std::visit([&](auto&& egress) { return egress.connect(peer); }, egress);
  co_await std::visit([](auto&& ingress) { return ingress.confirm(); }, ingress);
  router_.reset();

  std::clog << std::format(
      "{} | {}:{} | {}: {} -> {}\n",
      std::chrono::system_clock::now(),
      peer.host_,
      peer.port_,
      rname,
      iname_,
      ename
  );

  co_return egress;
}

Awaitable<void> Session::start(vo::Ingress const& vo, Socket s)
{
  auto ingress = adapter::tcp::create_ingress(vo, std::move(s));

  auto [ec, egress] = co_await redirect(handshake(ingress, vo.type_));

  if (ec) {
    co_await std::visit([ec](auto&& ingress) { return ingress.disconnect(ec); }, ingress);
    throw sys::system_error(ec);
  }

  auto [order, e0, e1] = co_await asio::experimental::make_parallel_group(
                             asio::co_spawn(ex_, bridge(ingress, *egress), asio::deferred),
                             asio::co_spawn(ex_, bridge(*egress, ingress), asio::deferred)
  )
                             .async_wait(asio::experimental::wait_for_one(), asio::use_awaitable);

  if (e0) std::rethrow_exception(e0);
  if (e1) std::rethrow_exception(e1);
}

}  // namespace pichi::actor
