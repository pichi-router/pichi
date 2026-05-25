#define BOOST_TEST_MODULE pichi balancer test

#include "utils.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <pichi/common/literals.hpp>
#include <pichi/service/balancer.hpp>
#include <pichi/vo/ingress.hpp>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

using namespace std::literals;
namespace rngs  = std::ranges;
namespace views = rngs::views;

namespace pichi::unit_test {

static auto const N = 100_sz;

static auto const ENDPOINTS = std::invoke([]() {
  auto ret = std::array<Endpoint, N>{};
  for (auto i = 0_u16; i < N; ++i) {
    ret[i] = makeEndpoint("localhost", i);
  }
  return ret;
});

static auto gen_vo(BalanceType type)
{
  return vo::Ingress{
      .type_ = AdapterType::TUNNEL,
      .opt_ =
          vo::TunnelOption{
                           .destinations_ = {rngs::begin(ENDPOINTS), rngs::end(ENDPOINTS)},
                           .balance_      = type
          },
      .name_ = "pichi"s,
  };
}

template <typename TestCase> void run_test_case(TestCase&& test, vo::Ingress const& vo)
{
  run_case([&, test = std::forward<TestCase>(test)](auto&& ex) mutable -> Awaitable<void> {
    co_await service::create_balancer(ex, vo);
    auto ec = co_await redirect(test(co_await service::get_balancer(ex, vo.name_)));
    service::remove_balancer(ex, vo.name_);
    BOOST_CHECK(!ec);
  });
}

BOOST_AUTO_TEST_SUITE(BALANCER)

BOOST_AUTO_TEST_CASE(select_Empty)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto vos = std::array{BalanceType::RANDOM, BalanceType::ROUND_ROBIN, BalanceType::LEAST_CONN} |
               views::transform([](auto type) {
                 return vo::Ingress{
                     .type_ = AdapterType::TUNNEL,
                     .opt_  = vo::TunnelOption{.destinations_ = {}, .balance_ = type}
                 };
               });
    for (auto&& vo : vos)
      BOOST_CHECK_EXCEPTION(
          co_await service::create_balancer(ex, vo),
          SystemError,
          verify_exception<PichiError::MISC>
      );
  });
}

BOOST_AUTO_TEST_CASE(RANDOM_select_Probability)
{
  static auto const K = 1000;
  run_test_case(
      [](auto balancer) -> Awaitable<void> {
        auto data = std::unordered_map<uint16_t, int>{};
        for (auto i = 0_sz; i < N * K; ++i) {
          auto endpoint = co_await balancer->select();
          ++data[endpoint.port_];
        }
        BOOST_CHECK_EQUAL(N, data.size());
        rngs::for_each(data | views::values, [delta = sqrt(N * K) / 2](auto times) {
          BOOST_CHECK_LE(abs(times - K), delta);
        });
      },
      gen_vo(BalanceType::RANDOM)
  );
}

BOOST_AUTO_TEST_CASE(ROUND_ROBIN_select)
{
  run_test_case(
      [](auto balancer) -> Awaitable<void> {
        for (auto i = 0_u16; i < N; ++i) {
          auto endpoint = co_await balancer->select();
          BOOST_CHECK_EQUAL(i, endpoint.port_);
        }

        for (auto i = 0_u16; i < N; ++i) {
          auto endpoint = co_await balancer->select();
          BOOST_CHECK_EQUAL(i, endpoint.port_);
        }
      },
      gen_vo(BalanceType::ROUND_ROBIN)
  );
}

BOOST_AUTO_TEST_CASE(LEAST_CONN_select)
{
  run_test_case(
      [](auto balancer) -> Awaitable<void> {
        auto data = std::unordered_set<uint16_t>{};

        for (auto i = 0_u16; i < N; ++i) {
          for (auto j = 0_u16; j < N; ++j) {
            auto endpoint = co_await balancer->select();
            data.emplace(endpoint.port_);
          }
          BOOST_CHECK_EQUAL(N, rngs::size(data));
          data.clear();
        }
      },
      gen_vo(BalanceType::LEAST_CONN)
  );
}

BOOST_AUTO_TEST_CASE(LEAST_CONN_release)
{
  run_test_case(
      [](auto balancer) -> Awaitable<void> {
        for (auto port = 0_u16; port < N; ++port) {
          auto endpoint = co_await balancer->select();
          BOOST_CHECK_EQUAL(port, endpoint.port_);
        }

        for (auto port = 0_u16; port < N; ++port) {
          balancer->release(makeEndpoint("localhost", port));
          auto endpoint = co_await balancer->select();
          BOOST_CHECK_EQUAL(port, endpoint.port_);
        }
      },
      gen_vo(BalanceType::LEAST_CONN)
  );
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
