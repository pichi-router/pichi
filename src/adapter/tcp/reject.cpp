#include <pichi/common/config.hpp>
// Include config.hpp first
#include <chrono>
#include <pichi/adapter/tcp/reject.hpp>
#include <pichi/common/asserts.hpp>
#include <random>

namespace asio = boost::asio;

using namespace std::literals;

namespace pichi::adapter::tcp {

static auto calc_delay(vo::RejectOption const& opt)
{
  switch (opt.mode_) {
  case DelayMode::RANDOM: {
    auto g = std::mt19937{std::random_device{}()};
    auto r = std::uniform_int_distribution<>{0, 300};
    return r(g) * 1s;
  }
  case DelayMode::FIXED:
    return *opt.delay_ * 1s;
  default:
    fail();
  }
}

RejectEgress::RejectEgress(vo::Egress const& vo, IOExecutor const& ex)
  : timer_{ex, calc_delay(std::get<vo::RejectOption>(*vo.opt_))}
{
}

[[noreturn]] Awaitable<size_t> RejectEgress::recv(MutableBuffer)
{
  fail("RejectEgress::recv shouldn't be invoked");
}

[[noreturn]] Awaitable<void> RejectEgress::send(ConstBuffer)
{
  fail("RejectEgress::send shouldn't be invoked");
}

Awaitable<void> RejectEgress::close()
{
  timer_.cancel();
  co_return;
}

Awaitable<void> RejectEgress::connect(Endpoint const&)
{
  auto ec = co_await redirect(timer_.async_wait(asio::use_awaitable));
  if (ec != asio::error::operation_aborted) fail("Force to reject connection");
}

}  // namespace pichi::adapter::tcp
