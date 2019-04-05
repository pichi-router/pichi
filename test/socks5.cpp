#define BOOST_TEST_MODULE pichi socks5 test

#include "config.h"
#include "utils.hpp"
#include <array>
#include <boost/test/unit_test.hpp>
#include <initializer_list>
#include <memory>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/socks5.hpp>
#include <pichi/test/socket.hpp>
#include <vector>

using namespace std;
using namespace pichi;
namespace asio = boost::asio;
namespace sys = boost::system;

using test::Socket;
using Adapter = pichi::net::Socks5Adapter<test::Stream>;

static asio::detail::Pull* pPull = nullptr;
static asio::detail::Push* pPush = nullptr;
static asio::yield_context yield = {*pPush, *pPull};

BOOST_AUTO_TEST_SUITE(SOCKS5)

BOOST_AUTO_TEST_CASE(readRemote_Handshake_Invalid_Version)
{
  for (auto i = 0; i < 0x100; ++i) {
    if (i == 0x05) continue;
    auto ver = static_cast<uint8_t>(i);
    auto socket = Socket{};
    auto ingress = make_unique<Adapter>(socket, true);

    socket.fill({ver, 0x01, 0x00});
    BOOST_CHECK_EXCEPTION(ingress->readRemote(yield), Exception,
                          verifyException<PichiError::BAD_PROTO>);
    BOOST_CHECK_EQUAL(0, socket.available());
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Handshake_Zero_Nmethod)
{
  auto socket = Socket{};
  auto ingress = make_unique<Adapter>(socket, true);

  socket.fill({0x05, 0x00});
  BOOST_CHECK_EXCEPTION(ingress->readRemote(yield), Exception,
                        verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EQUAL(0, socket.available());
}

BOOST_AUTO_TEST_CASE(readRemote_Handshake_Without_Acceptable_Method)
{
  for (auto i = 1; i < 0x100; ++i) {
    auto len = static_cast<uint8_t>(i);
    auto socket = Socket{};
    auto ingress = make_unique<Adapter>(socket, true);

    socket.fill({0x05, len});
    for (auto j = 0; j < len; ++j) socket.fill({len});
    BOOST_CHECK_EXCEPTION(ingress->readRemote(yield), Exception, verifyException<PichiError::MISC>);

    auto fact = array<uint8_t, 2>{};
    BOOST_CHECK_EQUAL(2, socket.available());
    socket.flush(fact);
    BOOST_CHECK_EQUAL(0x05, fact.front());
    BOOST_CHECK_EQUAL(0xff, fact.back());
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Handshake_With_Acceptable_Method)
{
  for (auto i = 1; i < 0x100; ++i) {
    auto socket = Socket{};
    auto ingress = make_unique<Adapter>(socket, true);

    socket.fill({0x05, 0xff});
    for (auto j = 0; j < 0x100; ++j)
      if (j == i - 1)
        socket.fill({0x00});
      else
        socket.fill({0xff});
    BOOST_CHECK_EXCEPTION(ingress->readRemote(yield), Exception, verifyException<PichiError::MISC>);

    auto fact = array<uint8_t, 2>{};
    BOOST_CHECK_EQUAL(2, socket.available());
    socket.flush(fact);
    BOOST_CHECK_EQUAL(0x05, fact.front());
    BOOST_CHECK_EQUAL(0x00, fact.back());
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Request_With_Invalid_Version)
{
  for (auto i = 0; i < 0x100; ++i) {
    if (i == 0x05) continue;
    auto socket = Socket{};
    auto ingress = make_unique<Adapter>(socket, true);

    socket.fill({
        0x05, 0x01, 0x00,                        // Handshake
        static_cast<uint8_t>(i),                 // VER
        0x01,                                    // CMD
        0x00,                                    // RSV
        0x01, 0x7f, 0x00, 0x00, 0x01, 0x01, 0xbb // Endpoint
    });

    BOOST_CHECK_EXCEPTION(ingress->readRemote(yield), Exception,
                          verifyException<PichiError::BAD_PROTO>);

    auto fact = array<uint8_t, 2>{};
    BOOST_CHECK_EQUAL(2, socket.available());
    socket.flush(fact);
    BOOST_CHECK_EQUAL(0x05, fact.front());
    BOOST_CHECK_EQUAL(0x00, fact.back());
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Request_With_Invalid_CMD)
{
  for (auto i = 0; i < 0x100; ++i) {
    if (i == 0x01) continue;
    auto socket = Socket{};
    auto ingress = make_unique<Adapter>(socket, true);

    socket.fill({
        0x05, 0x01, 0x00,                        // Handshake
        0x05,                                    // VER
        static_cast<uint8_t>(i),                 // CMD
        0x00,                                    // RSV
        0x01, 0x7f, 0x00, 0x00, 0x01, 0x01, 0xbb // Endpoint
    });

    BOOST_CHECK_EXCEPTION(ingress->readRemote(yield), Exception,
                          verifyException<PichiError::BAD_PROTO>);

    auto fact = array<uint8_t, 2>{};
    BOOST_CHECK_EQUAL(2, socket.available());
    socket.flush(fact);
    BOOST_CHECK_EQUAL(0x05, fact.front());
    BOOST_CHECK_EQUAL(0x00, fact.back());
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Request_With_Invalid_RSV)
{
  for (auto i = 1; i < 0x100; ++i) {
    auto socket = Socket{};
    auto ingress = make_unique<Adapter>(socket, true);

    socket.fill({
        0x05, 0x01, 0x00,                        // Handshake
        0x05,                                    // VER
        0x01,                                    // CMD
        static_cast<uint8_t>(i),                 // RSV
        0x01, 0x7f, 0x00, 0x00, 0x01, 0x01, 0xbb // Endpoint
    });

    BOOST_CHECK_EXCEPTION(ingress->readRemote(yield), Exception,
                          verifyException<PichiError::BAD_PROTO>);

    auto fact = array<uint8_t, 2>{};
    BOOST_CHECK_EQUAL(2, socket.available());
    socket.flush(fact);
    BOOST_CHECK_EQUAL(0x05, fact.front());
    BOOST_CHECK_EQUAL(0x00, fact.back());
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Correct)
{
  auto expect = net::makeEndpoint("::1"sv, "443"sv);
  auto otects = array<uint8_t, 19>{};

  auto socket = Socket{};
  auto ingress = make_unique<Adapter>(socket, true);

  socket.fill({0x05, 0x01, 0x00, 0x05, 0x01, 0x00});

  net::serializeEndpoint(expect, otects);
  socket.fill(otects);

  auto fact = ingress->readRemote(yield);

  BOOST_CHECK_EQUAL(2, socket.available());
  socket.flush({otects, 2});
  BOOST_CHECK_EQUAL(0x05, otects[0]);
  BOOST_CHECK_EQUAL(0x00, otects[1]);

  BOOST_CHECK(expect.type_ == fact.type_);
  BOOST_CHECK_EQUAL(expect.host_, fact.host_);
  BOOST_CHECK_EQUAL(expect.port_, fact.port_);
}

BOOST_AUTO_TEST_CASE(connect_Handshake_Invalid_Version)
{
  for (auto i = 0; i < 0x100; ++i) {
    if (i == 0x05) continue;
    auto socket = Socket{};
    auto egress = make_unique<Adapter>(socket);

    socket.fill({static_cast<uint8_t>(i), 0x00});
    BOOST_CHECK_EXCEPTION(egress->connect({}, {}, yield), Exception,
                          verifyException<PichiError::BAD_PROTO>);

    BOOST_CHECK_EQUAL(3, socket.available());
    auto req = array<uint8_t, 3>{};
    socket.flush(req);
    BOOST_CHECK_EQUAL(0x05, req[0]);
    BOOST_CHECK_EQUAL(0x01, req[1]);
    BOOST_CHECK_EQUAL(0x00, req[2]);
  }
}

BOOST_AUTO_TEST_CASE(connect_Handshake_Without_Acceptable_Method)
{
  for (auto i = 1; i < 0x100; ++i) {
    auto socket = Socket{};
    auto egress = make_unique<Adapter>(socket);

    socket.fill({0x05, static_cast<uint8_t>(i)});
    BOOST_CHECK_EXCEPTION(egress->connect({}, {}, yield), Exception,
                          verifyException<PichiError::BAD_PROTO>);

    BOOST_CHECK_EQUAL(3, socket.available());
    auto req = array<uint8_t, 3>{};
    socket.flush(req);
    BOOST_CHECK_EQUAL(0x05, req[0]);
    BOOST_CHECK_EQUAL(0x01, req[1]);
    BOOST_CHECK_EQUAL(0x00, req[2]);
  }
}

BOOST_AUTO_TEST_CASE(connect_Reply_With_Invalid_Version)
{
  auto expect = array<uint8_t, 25>{0x05, 0x01, 0x00, 0x05, 0x01, 0x00};
  auto remote = net::makeEndpoint("::1"sv, "443"sv);
  net::serializeEndpoint(remote, MutableBuffer<uint8_t>{expect} + 6);

  for (auto i = 0; i < 0x100; ++i) {
    if (i == 0x05) continue;
    auto socket = Socket{};
    auto egress = make_unique<Adapter>(socket);

    socket.fill({0x05, 0x00});
    socket.fill({static_cast<uint8_t>(i), 0x00, 0x00});
    BOOST_CHECK_EXCEPTION(egress->connect(remote, {}, yield), Exception,
                          verifyException<PichiError::BAD_PROTO>);

    BOOST_CHECK_EQUAL(25, socket.available());
    auto fact = array<uint8_t, 25>{};
    socket.flush(fact);
    BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
  }
}

BOOST_AUTO_TEST_CASE(connect_Reply_Connection_Failure)
{
  auto expect = array<uint8_t, 25>{0x05, 0x01, 0x00, 0x05, 0x01, 0x00};
  auto remote = net::makeEndpoint("::1"sv, "443"sv);
  net::serializeEndpoint(remote, MutableBuffer<uint8_t>{expect} + 6);

  for (auto i = 1; i < 0x100; ++i) {
    auto socket = Socket{};
    auto egress = make_unique<Adapter>(socket);

    socket.fill({0x05, 0x00});
    socket.fill({0x05, static_cast<uint8_t>(i), 0x00});
    BOOST_CHECK_EXCEPTION(egress->connect(remote, {}, yield), Exception,
                          verifyException<PichiError::CONN_FAILURE>);

    BOOST_CHECK_EQUAL(25, socket.available());
    auto fact = array<uint8_t, 25>{};
    socket.flush(fact);
    BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
  }
}

BOOST_AUTO_TEST_CASE(connect_Reply_With_Invalid_RSV)
{
  auto expect = array<uint8_t, 25>{0x05, 0x01, 0x00, 0x05, 0x01, 0x00};
  auto remote = net::makeEndpoint("::1"sv, "443"sv);
  net::serializeEndpoint(remote, MutableBuffer<uint8_t>{expect} + 6);

  for (auto i = 1; i < 0x100; ++i) {
    auto socket = Socket{};
    auto egress = make_unique<Adapter>(socket);

    socket.fill({0x05, 0x00});
    socket.fill({0x05, 0x00, static_cast<uint8_t>(i)});
    BOOST_CHECK_EXCEPTION(egress->connect(remote, {}, yield), Exception,
                          verifyException<PichiError::BAD_PROTO>);

    BOOST_CHECK_EQUAL(25, socket.available());
    auto fact = array<uint8_t, 25>{};
    socket.flush(fact);
    BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
  }
}

BOOST_AUTO_TEST_CASE(connect_Reply_Invalid_Endpoint)
{
  auto expect = array<uint8_t, 25>{0x05, 0x01, 0x00, 0x05, 0x01, 0x00};
  auto remote = net::makeEndpoint("::1"sv, "443"sv);
  net::serializeEndpoint(remote, MutableBuffer<uint8_t>{expect} + 6);

  auto socket = Socket{};
  auto egress = make_unique<Adapter>(socket);

  socket.fill({0x05, 0x00});
  socket.fill({0x05, 0x00, 0x00});
  socket.fill({0x00});
  BOOST_CHECK_EXCEPTION(egress->connect(remote, {}, yield), Exception,
                        verifyException<PichiError::BAD_PROTO>);

  BOOST_CHECK_EQUAL(25, socket.available());
  auto fact = array<uint8_t, 25>{};
  socket.flush(fact);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE(connect_Reply_Correct)
{
  auto expect = array<uint8_t, 25>{0x05, 0x01, 0x00, 0x05, 0x01, 0x00};
  auto remote = net::makeEndpoint("::1"sv, "443"sv);
  net::serializeEndpoint(remote, MutableBuffer<uint8_t>{expect} + 6);

  auto socket = Socket{};
  auto egress = make_unique<Adapter>(socket);

  socket.fill({0x05, 0x00});
  socket.fill({0x05, 0x00, 0x00});
  socket.fill({0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});

  egress->connect(remote, {}, yield);

  BOOST_CHECK_EQUAL(25, socket.available());
  auto fact = array<uint8_t, 25>{};
  socket.flush(fact);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE(confirm)
{
  auto socket = Socket{};
  auto ingress = make_unique<Adapter>(socket, true);

  ingress->confirm(yield);

  auto buf = array<uint8_t, 512>{};
  socket.flush({buf, 3});
  BOOST_CHECK_EQUAL(0x05, buf[0]);
  BOOST_CHECK_EQUAL(0x00, buf[1]);
  BOOST_CHECK_EQUAL(0x00, buf[2]);

  auto consumed = 0;
  auto len = socket.available();
  socket.flush({buf, len});
  net::parseEndpoint([src = ConstBuffer<uint8_t>{buf, len}, &consumed](auto dst) mutable {
    BOOST_CHECK(src.size() >= dst.size());
    copy_n(cbegin(src), dst.size(), begin(dst));
    src += dst.size();
    consumed += dst.size();
  });
  BOOST_CHECK_EQUAL(len, consumed);
}

BOOST_AUTO_TEST_CASE(disconnect)
{
  auto socket = Socket{};
  auto ingress = make_unique<Adapter>(socket, true);

  ingress->disconnect(yield);

  auto buf = array<uint8_t, 512>{};
  socket.flush({buf, 3});
  BOOST_CHECK_EQUAL(0x05, buf[0]);
  BOOST_CHECK_EQUAL(0x04, buf[1]);
  BOOST_CHECK_EQUAL(0x00, buf[2]);

  auto consumed = 0;
  auto len = socket.available();
  socket.flush({buf, len});
  net::parseEndpoint([src = ConstBuffer<uint8_t>{buf, len}, &consumed](auto dst) mutable {
    BOOST_CHECK(src.size() >= dst.size());
    copy_n(cbegin(src), dst.size(), begin(dst));
    src += dst.size();
    consumed += dst.size();
  });
  BOOST_CHECK_EQUAL(len, consumed);
}

BOOST_AUTO_TEST_SUITE_END()
