#include <algorithm>
#include <boost/asio/detail/throw_error.hpp>
#include <pichi/actor/detached.hpp>
#include <pichi/actor/listener.hpp>
#include <pichi/actor/session.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/service/balancer.hpp>
#include <ranges>

namespace asio  = boost::asio;
namespace ip    = asio::ip;
namespace rngs  = std::ranges;
namespace views = std::views;

namespace pichi::actor {

Awaitable<void> Listener::listen(Acceptor& ac)
{
  auto ex = strand_.get_inner_executor();
  if (vo_.type_ == AdapterType::TUNNEL) co_await service::create_balancer(ex, vo_);
  while (ac.is_open()) {
    auto [ec, s] = co_await redirect(ac.async_accept(asio::use_awaitable));
    if (ec == asio::error::operation_aborted)
      break;
    else
      asio::detail::throw_error(ec);

    co_await switch_to(strand_);
    asio::co_spawn(
        ex,
        [session = Session{ex, router_}, s = std::move(*s), &vo = vo_]() mutable {
          return session.start(vo, std::move(s));
        },
        detached
    );
  }
}

Listener::Listener(IOExecutor const& ex, RouterPtr const& router, vo::Ingress vo)
  : strand_{asio::make_strand(ex)}, router_{router}, vo_{std::move(vo)}
{
  auto v = vo_.bind_ | views::transform([ex](auto&& endpoint) {
             return Acceptor{
                 ex,
                 ip::tcp::endpoint{ip::make_address(endpoint.host_), endpoint.port_}
             };
           });
  acceptors_.assign(rngs::begin(v), rngs::end(v));
}

Listener::~Listener()
{
  // router_ is also a flag to indicate whether this listener is moved or not.
  if (router_ != nullptr && vo_.type_ == AdapterType::TUNNEL)
    service::remove_balancer(strand_, vo_.name_);
}

vo::Ingress const& Listener::vo() const { return vo_; }

void Listener::start()
{
  rngs::for_each(acceptors_, [ex = strand_.get_inner_executor(), this](auto&& acceptor) {
    asio::co_spawn(ex, listen(acceptor), detached);
  });
}

void Listener::reroute(RouterPtr const& router)
{
  asio::post(strand_, [this, router]() { router_ = std::move(router); });
}

}  // namespace pichi::actor
