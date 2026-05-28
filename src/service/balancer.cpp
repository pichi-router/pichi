#include "pichi/common/config.hpp"
#include <boost/asio/post.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/service/balancer.hpp>
#include <ranges>

namespace asio  = boost::asio;
namespace rngs  = std::ranges;
namespace views = rngs::views;

namespace pichi::service {

namespace balancer {

void Service::shutdown() noexcept
{
  balancers_.clear();
  strand_.reset();
}

Service::Service(asio::execution_context& ctx)
  : asio::detail::execution_context_service_base<Service>{ctx}
{
}

void Service::initialize(IOExecutor const& ex)
{
  std::call_once(flag_, [this](auto&& ex) { strand_.emplace(asio::make_strand(ex)); }, ex);
}

void Service::remove(std::string const& name)
{
  asio::post(*strand_, [this, name]() { balancers_.erase(name); });
}

Awaitable<BalancerPtr> Service::get(std::string const& name)
{
  co_await switch_to(*strand_);
  assertTrue(balancers_.contains(name));
  co_return balancers_.at(name);
}

Awaitable<void> Service::create(vo::Ingress const& vo)
{
  co_await switch_to(*strand_);
  balancers_.insert_or_assign(
      vo.name_,
      std::make_shared<Balancer>(strand_->get_inner_executor(), vo)
  );
}

Service& use_service(IOExecutor const& ex)
{
  return asio::use_service<Service>(asio::query(ex, asio::execution::context));
}

Random::Random(IOExecutor const& ex, vo::TunnelOption const& opt)
  : strand_{asio::make_strand(ex)},
    peers_{opt.destinations_},
    g_{std::invoke(std::random_device{})},
    rand_{0, rngs::size(peers_) - 1}
{
}

Awaitable<Endpoint> Random::select()
{
  co_await switch_to(strand_);
  co_return peers_[rand_(g_)];
}

void Random::release(Endpoint const&) {}

RoundRobin::RoundRobin(IOExecutor const& ex, vo::TunnelOption const& opt)
  : strand_{asio::make_strand(ex)}, peers_{opt.destinations_}, pos_{0_sz}
{
}

Awaitable<Endpoint> RoundRobin::select()
{
  co_await switch_to(strand_);
  auto ret = peers_[pos_];
  pos_     = (pos_ + 1) % rngs::size(peers_);
  co_return ret;
}

void RoundRobin::release(Endpoint const&) {}

LeastConn::LeastConn(IOExecutor const& ex, vo::TunnelOption const& opt)
  : strand_{asio::make_strand(ex)}, index_{}, iters_{}, peers_{opt.destinations_}
{
  rngs::for_each(views::iota(0_sz, rngs::size(peers_)), [this](auto i) {
    iters_[i] = index_.emplace(0_sz, i);
  });
}

Awaitable<Endpoint> LeastConn::select()
{
  co_await switch_to(strand_);

  auto it   = rngs::begin(index_);
  auto conn = it->first;
  auto pos  = it->second;
  index_.erase(it);
  iters_[pos] = index_.emplace(++conn, pos);
  co_return peers_[pos];
}

void LeastConn::release(Endpoint const& peer)
{
  asio::post(strand_, [=, this]() {
    for (auto pos = 0_sz; pos < rngs::size(peers_); ++pos)
      if (peers_[pos] == peer) {
        auto it   = iters_[pos];
        auto conn = it->first;
        index_.erase(it);
        iters_[pos] = index_.emplace(--conn, pos);
      }
  });
}

}  // namespace balancer

Balancer::Manner Balancer::initialize(IOExecutor const& ex, vo::Ingress const& vo)
{
  assertTrue(vo.type_ == AdapterType::TUNNEL);
  auto opt = std::get<vo::TunnelOption>(*vo.opt_);
  switch (opt.balance_) {
  case BalanceType::RANDOM:
    return balancer::Random{ex, opt};
  case BalanceType::ROUND_ROBIN:
    return balancer::RoundRobin{ex, opt};
  case BalanceType::LEAST_CONN:
    return balancer::LeastConn{ex, opt};
  default:
    fail();
  }
}

Balancer::Balancer(IOExecutor const& ex, vo::Ingress const& vo) : manner_{initialize(ex, vo)} {}

Awaitable<Endpoint> Balancer::select()
{
  co_return co_await std::visit([](auto&& manner) { return manner.select(); }, manner_);
}

void Balancer::release(Endpoint const& peer)
{
  std::visit([&](auto&& manner) { manner.release(peer); }, manner_);
}

Awaitable<void> create_balancer(IOExecutor const& ex, vo::Ingress const& vo)
{
  auto& svc = balancer::use_service(ex);
  svc.initialize(ex);
  co_await svc.create(vo);
}

void remove_balancer(IOExecutor const& ex, std::string const& name)
{
  balancer::use_service(ex).remove(name);
}

Awaitable<BalancerPtr> get_balancer(IOExecutor const& ex, std::string const& name)
{
  co_return co_await balancer::use_service(ex).get(name);
}

}  // namespace pichi::service
