#include "pichi/common/config.hpp"
#include <botan/hex.h>
#include <pichi/actor/detached.hpp>
#include <pichi/service/sentry.hpp>

using namespace std::literals;
namespace asio = boost::asio;
namespace sys  = boost::system;

namespace pichi::service {

SaltSentry::SaltSentry(asio::execution_context& ctx)
  : asio::detail::execution_context_service_base<SaltSentry>{ctx}
{
}

void SaltSentry::initialize(IOExecutor const& ex)
{
  std::call_once(
      flag_,
      [this](auto&& ex) {
        strand_ = asio::make_strand(ex);
        timer_  = asio::system_timer{ex};
        asio::co_spawn(ex, run(), actor::detached);
      },
      ex
  );
}

void SaltSentry::cancel()
{
  timer_.reset();
  strand_.reset();
}

Awaitable<void> SaltSentry::run()
{
  auto ec = sys::error_code{};
  while (true) {
    // TODO Adjust how long it expires
    timer_->expires_after(600s);
    co_await redirect(timer_->async_wait(asio::use_awaitable), ec);
    if (ec) break;
    co_await switch_to(*strand_);
    expiring_ = std::move(active_);
  }
  if (ec != asio::error::operation_aborted) asio::detail::throw_error(ec);
}

Awaitable<bool> SaltSentry::contains(ConstBuffer salt)
{
  co_await switch_to(*strand_);
  auto [it, inserted] = active_.insert(Botan::hex_encode(salt));
  co_return !inserted || expiring_.contains(*it);
}

Awaitable<void> SaltSentry::reset()
{
  co_await switch_to(*strand_);
  expiring_.clear();
  active_.clear();
}

void SaltSentry::shutdown() noexcept
{
  timer_.reset();
  strand_.reset();
}

SaltSentry& get_sentry(IOExecutor const& ex)
{
  auto&& sentry = asio::use_service<SaltSentry>(asio::query(ex, asio::execution::context));
  sentry.initialize(ex);
  return sentry;
}

}  // namespace pichi::service
