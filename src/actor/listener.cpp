#include <format>
#include <iostream>
#include <pichi/actor/detached.hpp>
#include <pichi/actor/listener.hpp>
#include <pichi/actor/session.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/enumerations.hpp>

namespace asio = boost::asio;
namespace ip   = asio::ip;

namespace pichi::actor {

Awaitable<void> Listener::listen(Acceptor& ac)
{
  auto ex = strand_.get_inner_executor();
  while (ac.is_open()) {
    auto [ec, s] = co_await redirect(ac.async_accept(asio::use_awaitable));
    if (ec) {
      if (ec != asio::error::operation_aborted)
        std::clog << std::format("ERROR: {}\n", ec.message());
      break;
    }

    co_await switch_to(strand_);
    asio::co_spawn(
        ex,
        [session = Session{ex, router_, name_}, s = std::move(*s), &vo = vo_]() mutable {
          return session.start(vo, std::move(s));
        },
        detached
    );
  }
}

Listener::Listener(
    IOExecutor const& ex, std::string const& name, RouterPtr const& router, vo::Ingress vo
)
  : strand_{asio::make_strand(ex)}, router_{router}, name_{name}, vo_{std::move(vo)}
{
}

void Listener::start()
{
  auto ex = strand_.get_inner_executor();
  for (auto&& endpoint : vo_.bind_) {
    acceptors_.emplace_back(
        ex,
        asio::ip::tcp::endpoint{ip::make_address(endpoint.host_), endpoint.port_}
    );
    asio::co_spawn(ex, listen(acceptors_.back()), detached);
  }
}

void Listener::reroute(RouterPtr const& router)
{
  asio::post(strand_, [this, router]() { router_ = std::move(router); });
}

}  // namespace pichi::actor
