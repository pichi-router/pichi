#define BOOST_TEST_MODULE pichi socks5 test

#include "pichi/common/config.hpp"
#include "utils.hpp"
#include <array>
#include <functional>
#include <pichi/actor/detached.hpp>
#include <pichi/adapter/tcp/socks5.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/test.hpp>
#include <ranges>
#include <vector>

using namespace std::literals;
namespace asio  = boost::asio;
namespace rngs  = std::ranges;
namespace sys   = boost::system;
namespace views = rngs::views;

namespace pichi::unit_test {

using Ingress = adapter::tcp::Socks5Ingress<TestSocket>;
using Egress  = adapter::tcp::Socks5Egress<TestSocket>;

static auto const USERNAME = "pichi"s;
static auto const PASSWORD = "pichi"s;
static auto const IVO_AUTH = vo::Ingress{
    .type_       = AdapterType::SOCKS5,
    .credential_ = vo::UpIngressCredential{.credential_ = {{USERNAME, PASSWORD}}}
};
static auto const EVO_AUTH = vo::Egress{
    .type_       = AdapterType::SOCKS5,
    .credential_ = vo::UpEgressCredential{.credential_ = {USERNAME, PASSWORD}}
};

static auto const ENDPOINT = makeEndpoint("::1", 443);

static auto const CLIENT_HDK = std::array{
    0x05_u8,  // VER
    0x01_u8,  // N-methods
    0x00_u8,  // No authentication
};

static auto const CLIENT_REQ = std::array{
    0x05_u8,  // Req VER
    0x01_u8,  // Req CMD
    0x00_u8,  // Req RSV
    0x04_u8,  // Req AYTP
    0x00_u8,  // IPv6 address
    0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8,
    0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x01_u8,
    0x01_u8,  // Port
    0xbb_u8,
};

static auto const SERVER_REP = std::array{
    0x05_u8,  // Rep VER
    0x00_u8,  // Rep REP
    0x00_u8,  // Rep RSV
    0x01_u8,  // Rep AYTP
    0x00_u8,  // IPv4 address
    0x00_u8,
    0x00_u8,
    0x00_u8,
    0x00_u8,  // Port
    0x00_u8,
};

BOOST_AUTO_TEST_SUITE(SOCKS5)

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Handshake_Invalid_Version)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto hdks = views::iota(0, 0x100) | views::filter([](auto i) { return i != 5; }) |
                views::transform([](uint8_t b) {
                  auto ret = CLIENT_HDK;
                  ret[0]   = b;
                  return ret;
                });
    for (auto&& hdk : hdks) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};
      co_await stream::write(client, hdk);
      BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_PROTO>);
    };
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Handshake_Zero_Method)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    auto buf = std::array{
        0x05_u8,  // VER
        0x00_u8,  // N-methods
    };
    co_await stream::write(client, buf);
    BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_PROTO>);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Handshake_Without_Acceptable_Method)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};
    auto ms      = views::iota(1, 0xff);
    auto buf     = std::vector<uint8_t>{
        0x05_u8,  // VER
        0xfe_u8,  // N-methods
    };
    buf.insert(rngs::end(buf), rngs::begin(ms), rngs::end(ms));

    co_await stream::write(client, buf);
    BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_AUTH_METHOD>);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Handshake_Without_Acceptable_Method_Credentials)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};
    auto ms      = views::iota(0, 0x100) | views::filter([](auto i) { return i != 2; });
    auto buf     = std::vector<uint8_t>{
        0x05_u8,  // VER
        0xff_u8,  // N-methods
    };
    buf.insert(rngs::end(buf), rngs::begin(ms), rngs::end(ms));

    co_await stream::write(client, buf);
    BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_AUTH_METHOD>);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Handshake_With_Acceptable_Method)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto buf = std::vector<uint8_t>{
        0x05_u8,  // VER
        0xff_u8,  // N-methods
        0x00_u8,  // No authentication
    };
    auto ms = views::iota(0, 0x100);
    buf.insert(rngs::end(buf), rngs::rbegin(ms), rngs::rend(ms));

    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    asio::co_spawn(
        ex,
        std::invoke([&]() -> Awaitable<void> {
          co_await ingress.read_remote();
          BOOST_CHECK(false);
        }),
        actor::detached
    );

    co_await stream::write(client, buf);
    BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL(0x05, buf[0]);
    BOOST_CHECK_EQUAL(0x00, buf[1]);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authenticate_With_Invalid_Version)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(0, 0x100) | views::filter([](auto b) { return b != 1; }) |
              views::transform([](uint8_t b) {
                auto ret = std::vector<uint8_t>{
                    0x05_u8,  // VER
                    0x01_u8,  // N-methods
                    0x02_u8,  // Username/Password
                    b,        // Auth VER
                };
                ret.push_back(static_cast<uint8_t>(rngs::size(USERNAME)));
                ret.insert(rngs::end(ret), rngs::begin(USERNAME), rngs::end(USERNAME));
                ret.push_back(static_cast<uint8_t>(rngs::size(PASSWORD)));
                ret.insert(rngs::end(ret), rngs::begin(PASSWORD), rngs::end(PASSWORD));
                return ret;
              });
    for (auto&& buf : bs) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{IVO_AUTH, client.peer()};
      co_await stream::write(client, buf);
      BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_PROTO>);

      BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL(0x05, buf[0]);
      BOOST_CHECK_EQUAL(0x02, buf[1]);
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authenticate_With_Empty_Username)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto buf = std::vector<uint8_t>{
        0x05_u8,  // VER
        0x01_u8,  // N-methods
        0x02_u8,  // Username/Password
        0x01_u8,  // Auth VER
        0x00_u8,  // Empty username
    };
    buf.push_back(static_cast<uint8_t>(rngs::size(PASSWORD)));
    buf.insert(rngs::end(buf), rngs::begin(PASSWORD), rngs::end(PASSWORD));

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};
    co_await stream::write(client, buf);
    BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_PROTO>);

    BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL(0x05, buf[0]);
    BOOST_CHECK_EQUAL(0x02, buf[1]);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authenticate_With_Empty_Password)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto buf = std::vector<uint8_t>{
        0x05_u8,  // VER
        0x01_u8,  // N-methods
        0x02_u8,  // Username/Password
        0x01_u8,  // Auth VER
    };
    buf.push_back(static_cast<uint8_t>(rngs::size(USERNAME)));
    buf.insert(rngs::end(buf), rngs::begin(USERNAME), rngs::end(USERNAME));
    buf.push_back(0x00_u8);  // Empty password

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};
    co_await stream::write(client, buf);
    BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_PROTO>);

    BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL(0x05, buf[0]);
    BOOST_CHECK_EQUAL(0x02, buf[1]);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authenticate_With_Nonexisting_User)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto buf = std::vector<uint8_t>{
        0x05_u8,  // VER
        0x01_u8,  // N-methods
        0x02_u8,  // Username/Password
        0x01_u8,  // Auth VER
        0x01_u8,  // Username length
        0x50_u8,  // Username
    };
    buf.push_back(static_cast<uint8_t>(rngs::size(PASSWORD)));
    buf.insert(rngs::end(buf), rngs::begin(PASSWORD), rngs::end(PASSWORD));

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};
    co_await stream::write(client, buf);
    BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::UNAUTHENTICATED>);

    BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL(0x05, buf[0]);
    BOOST_CHECK_EQUAL(0x02, buf[1]);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authenticate_With_Incorrect_Password)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto buf = std::vector<uint8_t>{
        0x05_u8,  // VER
        0x01_u8,  // N-methods
        0x02_u8,  // Username/Pasword
        0x01_u8,  // Auth VER
    };
    buf.push_back(static_cast<uint8_t>(rngs::size(USERNAME)));
    buf.insert(rngs::end(buf), rngs::begin(USERNAME), rngs::end(USERNAME));
    buf.push_back(0x01_u8);  // Password length
    buf.push_back(0x50_u8);  // Password

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};
    co_await stream::write(client, buf);
    BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::UNAUTHENTICATED>);

    BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL(0x05, buf[0]);
    BOOST_CHECK_EQUAL(0x02, buf[1]);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Request_With_Invalid_Version)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto reqs = views::iota(0, 0x100) | views::filter([](auto b) { return b != 5; }) |
                views::transform([](uint8_t b) {
                  auto ret = CLIENT_REQ;
                  ret[0]   = b;
                  return ret;
                });
    for (auto&& req : reqs) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};

      co_await stream::write(client, CLIENT_HDK);
      co_await stream::write(client, req);
      BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_PROTO>);

      auto buf = std::array<uint8_t, 64>{};
      BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL(0x05, buf[0]);
      BOOST_CHECK_EQUAL(0x00, buf[1]);
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Request_With_Invalid_CMD)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto reqs = views::iota(0, 0x100) | views::filter([](auto b) { return b != 1; }) |
                views::transform([](uint8_t b) {
                  auto ret = CLIENT_REQ;
                  ret[1]   = b;
                  return ret;
                });
    for (auto&& req : reqs) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};

      co_await stream::write(client, CLIENT_HDK);
      co_await stream::write(client, req);
      BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_PROTO>);

      auto buf = std::array<uint8_t, 64>{};
      BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL(0x05, buf[0]);
      BOOST_CHECK_EQUAL(0x00, buf[1]);
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Request_With_Invalid_RSV)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto reqs = views::iota(1, 0xff) | views::transform([](uint8_t b) {
                  auto ret = CLIENT_REQ;
                  ret[2]   = b;
                  return ret;
                });
    for (auto&& req : reqs) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};

      co_await stream::write(client, CLIENT_HDK);
      co_await stream::write(client, req);
      BOOST_CHECK_EXCEPTION(co_await ingress.read_remote(), SystemError, verify_exception<PichiError::BAD_PROTO>);

      auto buf = std::array<uint8_t, 64>{};
      BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL(0x05, buf[0]);
      BOOST_CHECK_EQUAL(0x00, buf[1]);
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Without_Authentication)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await stream::write(client, CLIENT_HDK);
    co_await stream::write(client, CLIENT_REQ);
    auto fact = co_await ingress.read_remote();

    auto buf = std::array<uint8_t, 64>{};
    BOOST_CHECK_EQUAL(2, co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL(0x05, buf[0]);
    BOOST_CHECK_EQUAL(0x00, buf[1]);

    BOOST_CHECK(ENDPOINT.type_ == fact.type_);
    BOOST_CHECK_EQUAL(ENDPOINT.host_, fact.host_);
    BOOST_CHECK_EQUAL(ENDPOINT.port_, fact.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_With_Correct_Credential)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto buf = std::vector<uint8_t>{
        0x05_u8,  // VER
        0x01_u8,  // N-method
        0x02_u8,  // Username/Password
        0x01_u8,  // Auth VER
    };
    buf.push_back(static_cast<uint8_t>(rngs::size(USERNAME)));
    buf.insert(rngs::end(buf), rngs::begin(USERNAME), rngs::end(USERNAME));
    buf.push_back(static_cast<uint8_t>(rngs::size(PASSWORD)));
    buf.insert(rngs::end(buf), rngs::begin(PASSWORD), rngs::end(PASSWORD));

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};

    co_await stream::write(client, buf);
    co_await stream::write(client, CLIENT_REQ);
    auto fact = co_await ingress.read_remote();

    BOOST_CHECK_EQUAL(4, co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL(0x05, buf[0]);
    BOOST_CHECK_EQUAL(0x02, buf[1]);
    BOOST_CHECK_EQUAL(0x01, buf[2]);
    BOOST_CHECK_EQUAL(0x00, buf[3]);

    BOOST_CHECK(ENDPOINT.type_ == fact.type_);
    BOOST_CHECK_EQUAL(ENDPOINT.host_, fact.host_);
    BOOST_CHECK_EQUAL(ENDPOINT.port_, fact.port_);
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Handshake_Invalid_Version)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(0, 0x100) | views::filter([](auto b) { return b != 5; }) |
              views::transform([](uint8_t b) {
                return std::vector<uint8_t>{
                    b,        // VER
                    0x00_u8,  // No authentication
                };
              });
    for (auto&& buf : bs) {
      auto server = TestSocket{ex};
      auto egress = Egress{{}, server.peer()};

      co_await stream::write(server, buf);
      BOOST_CHECK_EXCEPTION(co_await egress.connect({}), SystemError, verify_exception<PichiError::BAD_PROTO>);

      auto req = std::array<uint8_t, 64>{};
      BOOST_CHECK_EQUAL(3, co_await stream::read_some(server, req));
      BOOST_CHECK_EQUAL(0x05, req[0]);
      BOOST_CHECK_EQUAL(0x01, req[1]);
      BOOST_CHECK_EQUAL(0x00, req[2]);
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Handshake_Without_Acceptable_Method)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(1, 0xff) | views::transform([](uint8_t b) {
                return std::vector<uint8_t>{
                    0x05_u8,  // VER
                    b,        // Method
                };
              });
    for (auto&& buf : bs) {
      auto server = TestSocket{ex};
      auto egress = Egress{{}, server.peer()};

      co_await stream::write(server, buf);
      BOOST_CHECK_EXCEPTION(co_await egress.connect({}), SystemError, verify_exception<PichiError::BAD_AUTH_METHOD>);

      auto req = std::array<uint8_t, 64>{};
      BOOST_CHECK_EQUAL(3, co_await stream::read_some(server, req));
      BOOST_CHECK_EQUAL(0x05, req[0]);
      BOOST_CHECK_EQUAL(0x01, req[1]);
      BOOST_CHECK_EQUAL(0x00, req[2]);
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Handshake_Without_Acceptable_Method_Credential)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(0, 0x100) | views::filter([](auto b) { return b != 2; }) |
              views::transform([](uint8_t b) {
                return std::vector<uint8_t>{
                    0x05,  // VER
                    b,     // Method
                };
              });
    for (auto&& buf : bs) {
      auto server = TestSocket{ex};
      auto egress = Egress{EVO_AUTH, server.peer()};

      co_await stream::write(server, buf);
      BOOST_CHECK_EXCEPTION(co_await egress.connect({}), SystemError, verify_exception<PichiError::BAD_AUTH_METHOD>);

      auto req = std::array<uint8_t, 64>{};
      BOOST_CHECK_EQUAL(3, co_await stream::read_some(server, req));
      BOOST_CHECK_EQUAL(0x05, req[0]);
      BOOST_CHECK_EQUAL(0x01, req[1]);
      BOOST_CHECK_EQUAL(0x02, req[2]);
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Authentication_With_Invalid_Version)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(0, 0x100) | views::filter([](auto b) { return b != 1; }) |
              views::transform([](uint8_t b) {
                return std::vector<uint8_t>{
                    0x05_u8,  // VER
                    0x02_u8,  // Username/Password
                    b,        // Auth VER
                    0x00_u8,  // Auth OK
                };
              });
    for (auto&& buf : bs) {
      auto server = TestSocket{ex};
      auto egress = Egress{EVO_AUTH, server.peer()};

      co_await stream::write(server, buf);
      BOOST_CHECK_EXCEPTION(co_await egress.connect({}), SystemError, verify_exception<PichiError::BAD_PROTO>);

      auto req = std::array<uint8_t, 64>{};

      co_await stream::read(server, {req, 4});
      BOOST_CHECK_EQUAL(0x05, req[0]);
      BOOST_CHECK_EQUAL(0x01, req[1]);
      BOOST_CHECK_EQUAL(0x02, req[2]);
      BOOST_CHECK_EQUAL(0x01, req[3]);

      co_await stream::read(server, {req, 12});
      BOOST_CHECK_EQUAL(rngs::size(USERNAME), req[0]);
      BOOST_CHECK(rngs::equal(req | views::drop(1) | views::take(req[0]), USERNAME));
      BOOST_CHECK_EQUAL(rngs::size(PASSWORD), req[6]);
      BOOST_CHECK(rngs::equal(req | views::drop(7) | views::take(req[6]), PASSWORD));
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Authentication_Failed)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(1, 0xff) | views::transform([](uint8_t b) {
                return std::vector<uint8_t>{
                    0x05_u8,  // VER
                    0x02_u8,  // Username/Password
                    0x01_u8,  // Auth VER
                    b,        // Auth status
                };
              });
    for (auto&& buf : bs) {
      auto server = TestSocket{ex};
      auto egress = Egress{EVO_AUTH, server.peer()};

      co_await stream::write(server, buf);
      BOOST_CHECK_EXCEPTION(co_await egress.connect({}), SystemError, verify_exception<PichiError::UNAUTHENTICATED>);

      auto req = std::array<uint8_t, 64>{};

      co_await stream::read(server, {req, 4});
      BOOST_CHECK_EQUAL(0x05, req[0]);
      BOOST_CHECK_EQUAL(0x01, req[1]);
      BOOST_CHECK_EQUAL(0x02, req[2]);
      BOOST_CHECK_EQUAL(0x01, req[3]);

      co_await stream::read(server, {req, 12});
      BOOST_CHECK_EQUAL(rngs::size(USERNAME), req[0]);
      BOOST_CHECK(rngs::equal(req | views::drop(1) | views::take(req[0]), USERNAME));
      BOOST_CHECK_EQUAL(rngs::size(PASSWORD), req[6]);
      BOOST_CHECK(rngs::equal(req | views::drop(7) | views::take(req[6]), PASSWORD));
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Reply_With_Invalid_Version)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(0, 0x100) | views::filter([](auto b) { return b != 5; }) |
              views::transform([](uint8_t b) {
                return std::vector<uint8_t>{
                    0x05_u8,  // VER
                    0x00_u8,  // No authentication
                    b,        // Rep VER
                    0x00_u8,  // Rep REP
                    0x00_u8,  // Rep RSV
                };
              });
    for (auto&& buf : bs) {
      auto server = TestSocket{ex};
      auto egress = Egress{{}, server.peer()};

      co_await stream::write(server, buf);
      BOOST_CHECK_EXCEPTION(co_await egress.connect(ENDPOINT), SystemError, verify_exception<PichiError::BAD_PROTO>);

      auto data = std::array<uint8_t, 64>{};

      BOOST_CHECK_EQUAL(
          rngs::size(CLIENT_HDK) + rngs::size(CLIENT_REQ),
          co_await stream::read_some(server, data)
      );

      auto hdk = data | views::take(rngs::size(CLIENT_HDK));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(CLIENT_HDK),
          rngs::end(CLIENT_HDK),
          rngs::begin(hdk),
          rngs::end(hdk)
      );

      auto req = data | views::drop(rngs::size(CLIENT_HDK)) | views::take(rngs::size(CLIENT_REQ));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(CLIENT_REQ),
          rngs::end(CLIENT_REQ),
          rngs::begin(req),
          rngs::end(req)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Reply_Connection_Failure)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(1, 0xff) | views::transform([](uint8_t b) {
                return std::vector<uint8_t>{
                    0x05_u8,  // VER
                    0x00_u8,  // No authentication
                    0x05_u8,  // Rep VER
                    b,        // Rep REP
                    0x00_u8,  // Rep RSV
                };
              });
    for (auto&& buf : bs) {
      auto server = TestSocket{ex};
      auto egress = Egress{{}, server.peer()};

      co_await stream::write(server, buf);
      BOOST_CHECK_EXCEPTION(co_await egress.connect(ENDPOINT), SystemError, verify_exception<PichiError::CONN_FAILURE>);

      auto data = std::array<uint8_t, 64>{};

      BOOST_CHECK_EQUAL(
          rngs::size(CLIENT_HDK) + rngs::size(CLIENT_REQ),
          co_await stream::read_some(server, data)
      );

      auto hdk = data | views::take(rngs::size(CLIENT_HDK));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(CLIENT_HDK),
          rngs::end(CLIENT_HDK),
          rngs::begin(hdk),
          rngs::end(hdk)
      );

      auto req = data | views::drop(rngs::size(CLIENT_HDK)) | views::take(rngs::size(CLIENT_REQ));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(CLIENT_REQ),
          rngs::end(CLIENT_REQ),
          rngs::begin(req),
          rngs::end(req)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Reply_With_Invalid_RSV)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto bs = views::iota(1, 0xff) | views::transform([](uint8_t b) {
                return std::vector<uint8_t>{
                    0x05_u8,  // VER
                    0x00_u8,  // No authentication
                    0x05_u8,  // Rep VER
                    0x00_u8,  // Rep REP
                    b,        // Rep RSV
                };
              });
    for (auto&& buf : bs) {
      auto server = TestSocket{ex};
      auto egress = Egress{{}, server.peer()};

      co_await stream::write(server, buf);
      BOOST_CHECK_EXCEPTION(co_await egress.connect(ENDPOINT), SystemError, verify_exception<PichiError::BAD_PROTO>);

      auto data = std::array<uint8_t, 64>{};

      BOOST_CHECK_EQUAL(
          rngs::size(CLIENT_HDK) + rngs::size(CLIENT_REQ),
          co_await stream::read_some(server, data)
      );

      auto hdk = data | views::take(rngs::size(CLIENT_HDK));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(CLIENT_HDK),
          rngs::end(CLIENT_HDK),
          rngs::begin(hdk),
          rngs::end(hdk)
      );

      auto req = data | views::drop(rngs::size(CLIENT_HDK)) | views::take(rngs::size(CLIENT_REQ));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(CLIENT_REQ),
          rngs::end(CLIENT_REQ),
          rngs::begin(req),
          rngs::end(req)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Reply_Invalid_Endpoint)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto server = TestSocket{ex};
    auto egress = Egress{{}, server.peer()};

    co_await stream::write(
        server,
        std::array{
            0x05_u8,  // VER
            0x00_u8,  // No authentication
            0x05_u8,  // Rep VER
            0x00_u8,  // Rep REP
            0x00_u8,  // Rep RSV
            0x00_u8,  // Invalid endpoint
        }
    );

    BOOST_CHECK_EXCEPTION(co_await egress.connect(ENDPOINT), SystemError, verify_exception<PichiError::BAD_PROTO>);

    auto data = std::array<uint8_t, 64>{};

    BOOST_CHECK_EQUAL(
        rngs::size(CLIENT_HDK) + rngs::size(CLIENT_REQ),
        co_await stream::read_some(server, data)
    );

    auto hdk = data | views::take(rngs::size(CLIENT_HDK));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CLIENT_HDK),
        rngs::end(CLIENT_HDK),
        rngs::begin(hdk),
        rngs::end(hdk)
    );

    auto req = data | views::drop(rngs::size(CLIENT_HDK)) | views::take(rngs::size(CLIENT_REQ));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CLIENT_REQ),
        rngs::end(CLIENT_REQ),
        rngs::begin(req),
        rngs::end(req)
    );
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Reply_Correct)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto server = TestSocket{ex};
    auto egress = Egress{{}, server.peer()};

    co_await stream::write(
        server,
        std::array{
            0x05_u8,  // VER
            0x00_u8,  // No authentication
        }
    );
    co_await stream::write(server, SERVER_REP);

    co_await egress.connect(ENDPOINT);

    auto data = std::array<uint8_t, 64>{};

    BOOST_CHECK_EQUAL(
        rngs::size(CLIENT_HDK) + rngs::size(CLIENT_REQ),
        co_await stream::read_some(server, data)
    );

    auto hdk = data | views::take(rngs::size(CLIENT_HDK));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CLIENT_HDK),
        rngs::end(CLIENT_HDK),
        rngs::begin(hdk),
        rngs::end(hdk)
    );

    auto req = data | views::drop(rngs::size(CLIENT_HDK)) | views::take(rngs::size(CLIENT_REQ));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CLIENT_REQ),
        rngs::end(CLIENT_REQ),
        rngs::begin(req),
        rngs::end(req)
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_confirm)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await ingress.confirm();

    auto buf  = std::array<uint8_t, 64>{};
    auto fact = buf | views::take(co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(SERVER_REP),
        rngs::end(SERVER_REP),
        rngs::begin(fact),
        rngs::end(fact)
    );
  });
}

BOOST_AUTO_TEST_CASE(Egress_disconnect_For_Named_Failures)
{
  static auto const FAILURE_MAP = std::unordered_map<PichiError, std::vector<uint8_t>>{
      {   PichiError::CONN_FAILURE,
       {
       0x05_u8,  // Rep VER
       0x04_u8,  // Rep REP Host unreachable
       0x00_u8,  // Rep RSV
       0x01_u8,  // Rep AYTP
       0x00_u8,  // Rep endpoint
       0x00_u8,
       0x00_u8,
       0x00_u8,
       0x00_u8,
       0x00_u8,
       }},
      {PichiError::BAD_AUTH_METHOD,
       {
       0x05_u8,  // VER
       0xff_u8,  // Authentication failed
       }},
      {PichiError::UNAUTHENTICATED,
       {
       0x01_u8,  // Auth VER
       0xff_u8,  // Auth STATUS
       }}
  };
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto&& item : FAILURE_MAP) {
      auto [e, expect] = item;
      auto client      = TestSocket(ex);
      auto ingress     = Ingress{{}, client.peer()};

      co_await ingress.disconnect(make_error_code(e));

      auto buf  = std::array<uint8_t, 64>{};
      auto fact = buf | views::take(co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect_For_Unnamed_Failures)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto e :
         {PichiError::OK,
          PichiError::BAD_PROTO,
          PichiError::CRYPTO_ERROR,
          PichiError::BUFFER_OVERFLOW,
          PichiError::BAD_JSON,
          PichiError::SEMANTIC_ERROR,
          PichiError::RES_IN_USE,
          PichiError::RES_LOCKED,
          PichiError::MISC}) {
      auto expect = std::array{0xff_u8};

      auto client = TestSocket{ex};
      auto server = client.peer();

      co_await stream::write(server, expect);

      auto ingress = Ingress{{}, std::move(server)};
      co_await ingress.disconnect(make_error_code(e));

      auto buf  = std::array<uint8_t, 64>{};
      auto fact = buf | views::take(co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect_For_Specific_System_Error)
{
  static auto const TABLE = std::vector<std::pair<sys::error_code, uint8_t>>{
      {asio::error::address_family_not_supported, 0x08_u8},
      {          asio::error::connection_refused, 0x05_u8},
      {            asio::error::connection_reset, 0x05_u8},
      {              asio::error::host_not_found, 0x04_u8},
      {            asio::error::host_unreachable, 0x04_u8},
      {                asio::error::network_down, 0x03_u8},
      {         asio::error::network_unreachable, 0x03_u8},
      {                   asio::error::timed_out, 0x04_u8},
  };
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto&& item : TABLE) {
      auto [ec, b] = item;
      auto expect  = SERVER_REP;
      expect[1]    = b;

      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};

      co_await ingress.disconnect(ec);

      auto buf  = std::array<uint8_t, 64>{};
      auto fact = buf | views::take(co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect_For_Unused_System_Error)
{
  static auto const ECS = std::vector<sys::error_code>{
      asio::error::access_denied,
      asio::error::address_in_use,
      asio::error::already_connected,
      asio::error::bad_descriptor,
      asio::error::broken_pipe,
      asio::error::connection_aborted,
      asio::error::eof,
      asio::error::fault,
      asio::error::fd_set_failure,
      asio::error::host_not_found_try_again,
      asio::error::in_progress,
      asio::error::interrupted,
      asio::error::invalid_argument,
      asio::error::message_size,
      asio::error::no_buffer_space,
      asio::error::no_data,
      asio::error::no_descriptors,
      asio::error::no_memory,
      asio::error::no_permission,
      asio::error::no_protocol_option,
      asio::error::no_recovery,
      asio::error::no_such_device,
      asio::error::not_connected,
      asio::error::not_found,
      asio::error::not_socket,
      asio::error::operation_aborted,
      asio::error::operation_not_supported,
      asio::error::service_not_found,
      asio::error::shut_down,
      asio::error::socket_type_not_supported,
      asio::error::try_again,
      asio::error::would_block,
  };
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto ec : ECS) {
      auto expect = std::array{0xff_u8};

      auto client = TestSocket{ex};
      auto server = client.peer();

      co_await stream::write(server, expect);

      auto ingress = Ingress{{}, std::move(server)};
      co_await ingress.disconnect(ec);

      auto buf  = std::array<uint8_t, 64>{};
      auto fact = buf | views::take(co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL_COLLECTIONS(
          rngs::begin(expect),
          rngs::end(expect),
          rngs::begin(fact),
          rngs::end(fact)
      );
    }
  });
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
