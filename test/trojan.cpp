#define BOOST_TEST_MODULE pichi trojan test

#include "utils.hpp"
#include <algorithm>
#include <array>
#include <pichi/adapter/tcp/trojan.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/test.hpp>
#include <ranges>

using namespace std::literals;
namespace asio  = boost::asio;
namespace rngs  = std::ranges;
namespace sys   = boost::system;
namespace views = rngs::views;

namespace pichi::unit_test {

using Ingress = adapter::tcp::TrojanIngress<TestSocket>;
using Egress  = adapter::tcp::TrojanEgress<TestSocket>;

static auto const PASSWORD   = "pichi"s;
static auto const PWD_LENGTH = 56_sz;

static auto FALIED_EP  = makeEndpoint("localhost", "80");
static auto CORRECT_EP = makeEndpoint("127.0.0.1", "443");

static auto const CORRECT_DATA = std::array{
    // Hashed password
    0x65_u8, 0x38_u8, 0x65_u8, 0x32_u8, 0x34_u8, 0x63_u8, 0x62_u8, 0x30_u8, 0x61_u8, 0x34_u8,
    0x31_u8, 0x65_u8, 0x34_u8, 0x63_u8, 0x65_u8, 0x66_u8, 0x38_u8, 0x32_u8, 0x64_u8, 0x36_u8,
    0x64_u8, 0x64_u8, 0x32_u8, 0x61_u8, 0x66_u8, 0x35_u8, 0x32_u8, 0x64_u8, 0x33_u8, 0x35_u8,
    0x37_u8, 0x66_u8, 0x33_u8, 0x35_u8, 0x33_u8, 0x36_u8, 0x62_u8, 0x63_u8, 0x63_u8, 0x61_u8,
    0x33_u8, 0x61_u8, 0x65_u8, 0x63_u8, 0x35_u8, 0x63_u8, 0x36_u8, 0x63_u8, 0x66_u8, 0x30_u8,
    0x63_u8, 0x36_u8, 0x38_u8, 0x38_u8, 0x38_u8, 0x30_u8,
    0x0d_u8,  // '\r'
    0x0a_u8,  // '\n'
    0x01_u8,  // CMD
    0x01_u8,  // CORRECT_EP
    0x7f_u8, 0x00_u8, 0x00_u8, 0x01_u8, 0x01_u8, 0xbb_u8,
    0x0d_u8,  // '\r'
    0x0a_u8,  // '\n'
};

static auto const IVO = vo::Ingress{
    .type_       = AdapterType::HTTP,
    .credential_ = vo::TrojanIngressCredential{.credential_ = {PASSWORD}},
    .opt_        = vo::TrojanOption{.remote_ = FALIED_EP}
};

static auto const EVO = vo::Egress{
    .type_ = AdapterType::HTTP, .credential_ = vo::TrojanEgressCredential{.credential_ = PASSWORD}
};

BOOST_AUTO_TEST_SUITE(TROJAN)

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Correct_Stream)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto buf     = std::array<uint8_t, 1024>{};
    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO, client.peer()};

    co_await stream::write(client, CORRECT_DATA);
    co_await stream::close(client);

    BOOST_CHECK(CORRECT_EP == co_await ingress.read_remote());
    BOOST_CHECK_EXCEPTION(co_await ingress.recv(buf), SystemError, verify_exception<asio::error::eof>);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Simulate_HTTP_Server)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto i = 1_sz; i < PWD_LENGTH + 3; ++i) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{IVO, client.peer()};

      auto expect = CORRECT_DATA | views::take(i);
      co_await stream::write(client, expect);
      co_await stream::close(client);

      BOOST_CHECK(FALIED_EP == co_await ingress.read_remote());

      auto buf  = std::array<uint8_t, 1024>{};
      auto fact = buf | views::take(co_await ingress.recv(buf));

      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Unauthenticated)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto expect = CORRECT_DATA;
    rngs::fill(expect | views::take(PWD_LENGTH), 0_u8);

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO, client.peer()};

    co_await stream::write(client, expect);
    co_await stream::close(client);

    BOOST_CHECK(FALIED_EP == co_await ingress.read_remote());

    auto buf  = std::array<uint8_t, 1024>{};
    auto fact = buf | views::take(co_await ingress.recv(buf));

    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(expect),
        rngs::end(expect),
        rngs::begin(fact),
        rngs::end(fact)
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Invalid_Char_For_The_First_CR)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto ds = views::iota(0, 0x100) | views::filter([](auto b) { return '\r' != b; }) |
              views::transform([](uint8_t c) {
                auto data        = CORRECT_DATA;
                data[PWD_LENGTH] = c;
                return data;
              });
    for (auto&& expect : ds) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{IVO, client.peer()};

      co_await stream::write(client, expect);
      co_await stream::close(client);

      BOOST_CHECK(FALIED_EP == co_await ingress.read_remote());

      auto buf  = std::array<uint8_t, 1024>{};
      auto fact = buf | views::take(co_await ingress.recv(buf));

      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Invalid_Char_For_The_Second_CR)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto ds = views::iota(0, 0x100) | views::filter([](auto b) { return '\r' != b; }) |
              views::transform([](uint8_t c) {
                auto data                  = CORRECT_DATA;
                data[rngs::size(data) - 2] = c;
                return data;
              });
    for (auto&& expect : ds) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{IVO, client.peer()};

      co_await stream::write(client, expect);
      co_await stream::close(client);

      BOOST_CHECK(FALIED_EP == co_await ingress.read_remote());

      auto buf  = std::array<uint8_t, 1024>{};
      auto fact = buf | views::take(co_await ingress.recv(buf));

      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Invalid_Char_For_The_First_LF)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto ds = views::iota(0, 0x100) | views::filter([](auto b) { return '\n' != b; }) |
              views::transform([](uint8_t c) {
                auto data            = CORRECT_DATA;
                data[PWD_LENGTH + 1] = c;
                return data;
              });
    for (auto&& expect : ds) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{IVO, client.peer()};

      co_await stream::write(client, expect);
      co_await stream::close(client);

      BOOST_CHECK(FALIED_EP == co_await ingress.read_remote());

      auto buf  = std::array<uint8_t, 1024>{};
      auto fact = buf | views::take(co_await ingress.recv(buf));

      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Invalid_Char_For_The_Second_LF)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto ds = views::iota(0, 0x100) | views::filter([](auto b) { return '\n' != b; }) |
              views::transform([](uint8_t c) {
                auto data                  = CORRECT_DATA;
                data[rngs::size(data) - 1] = c;
                return data;
              });
    for (auto&& expect : ds) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{IVO, client.peer()};

      co_await stream::write(client, expect);
      co_await stream::close(client);

      BOOST_CHECK(FALIED_EP == co_await ingress.read_remote());

      auto buf  = std::array<uint8_t, 1024>{};
      auto fact = buf | views::take(co_await ingress.recv(buf));

      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Invalid_CMD)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto ds = views::iota(0, 0x100) | views::filter([](auto b) { return 1 != b; }) |
              views::transform([](uint8_t c) {
                auto data            = CORRECT_DATA;
                data[PWD_LENGTH + 2] = c;
                return data;
              });
    for (auto&& expect : ds) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{IVO, client.peer()};

      co_await stream::write(client, expect);
      co_await stream::close(client);

      BOOST_CHECK(FALIED_EP == co_await ingress.read_remote());

      auto buf  = std::array<uint8_t, 1024>{};
      auto fact = buf | views::take(co_await ingress.recv(buf));

      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Stream_Separated_From_Trojan_Request)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto buf     = std::array<uint8_t, 1024>{};
    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO, client.peer()};

    for (auto i = 0_sz; i < rngs::size(CORRECT_DATA); ++i)
      co_await stream::write(client, CORRECT_DATA | views::drop(i) | views::take(1));
    co_await stream::close(client);

    BOOST_CHECK(CORRECT_EP == co_await ingress.read_remote());
    BOOST_CHECK_EXCEPTION(co_await ingress.recv(buf), SystemError, verify_exception<asio::error::eof>);
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Correct_Request)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto server = TestSocket{ex};
    auto egress = Egress{EVO, server.peer()};

    co_await egress.connect(CORRECT_EP);

    auto buf  = std::array<uint8_t, 1024>{};
    auto fact = buf | views::take(co_await stream::read_some(server, buf));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CORRECT_DATA),
        rngs::end(CORRECT_DATA),
        rngs::begin(fact),
        rngs::end(fact)
    );
  });
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
