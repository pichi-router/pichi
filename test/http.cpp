#define BOOST_TEST_MODULE pichi http test

#include "config.h"
#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/http.hpp>
#include <pichi/test/socket.hpp>

using namespace std;
using namespace pichi;
namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace sys = boost::system;

using test::Socket;
using HttpIngress = net::HttpIngress<test::Stream>;
using HttpEgress = net::HttpEgress<test::Stream>;

static asio::detail::Pull* pPull = nullptr;
static asio::detail::Push* pPush = nullptr;
static asio::yield_context yield = {*pPush, *pPull};

static auto const HTTPS_ENDPOINT = net::makeEndpoint("localhost"sv, "443"sv);
static auto const HTTP_ENDPOINT = net::makeEndpoint("localhost"sv, "80"sv);
static auto const BEGIN_HTTPS_HDR = ConstBuffer<uint8_t>{"CONNECT localhost:443 HTTP/1.1\r\n"sv};
static auto const BEGIN_HTTP_HDR = ConstBuffer<uint8_t>{"GET http://localhost/ HTTP/1.1\r\n"sv};
static auto const BEGIN_NORMAL_HTTP_HDR = ConstBuffer<uint8_t>{"GET / HTTP/1.1\r\n"sv};
static auto const HOST_FIELD = ConstBuffer<uint8_t>{"Host: localhost\r\n"sv};
static auto const END_HTTP_HDR = ConstBuffer<uint8_t>{"\r\n"sv};

template <bool isRequest>
static void verifyField(http::header<isRequest> const& h, http::field field, string_view content)
{
  BOOST_CHECK(h.find(field) != cend(h));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(content), cend(content), cbegin(h[field]), cend(h[field]));
}

BOOST_AUTO_TEST_SUITE(HTTP)

BOOST_AUTO_TEST_CASE(readRemote_Invalid_HTTP_Header)
{
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(ConstBuffer<uint8_t>{"Not a legal http header"sv});
  BOOST_CHECK_EXCEPTION(ingress.readRemote(yield), sys::system_error,
                        verifyException<http::error::bad_version>);
}

BOOST_AUTO_TEST_CASE(readRemote_Tunnel_Correct)
{
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_HTTPS_HDR);
  socket.fill(END_HTTP_HDR);

  auto fact = ingress.readRemote(yield);

  BOOST_CHECK(HTTPS_ENDPOINT.type_ == fact.type_);
  BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.host_, fact.host_);
  BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.port_, fact.port_);

  auto buf = array<uint8_t, 16>{};
  BOOST_CHECK_EXCEPTION(ingress.recv(buf, yield), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(readRemote_Relay_With_Host_Field_Only)
{
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_NORMAL_HTTP_HDR);
  socket.fill(HOST_FIELD);
  socket.fill(END_HTTP_HDR);
  auto remote = ingress.readRemote(yield);

  BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
  BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
  BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);
}

BOOST_AUTO_TEST_CASE(readRemote_Relay_With_Both_Fields)
{
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_HTTP_HDR);
  socket.fill(HOST_FIELD);
  socket.fill(END_HTTP_HDR);
  auto remote = ingress.readRemote(yield);

  BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
  BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
  BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);
}

BOOST_AUTO_TEST_CASE(readRemote_Relay_Without_Host_Field)
{
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_HTTP_HDR);
  socket.fill(END_HTTP_HDR);

  BOOST_CHECK_EXCEPTION(ingress.readRemote(yield), Exception,
                        verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Tunnel_With_Sticky_Content)
{
  auto sticky = "sticky content"sv;
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_HTTPS_HDR);
  socket.fill(END_HTTP_HDR);
  socket.fill(ConstBuffer<uint8_t>{sticky});
  ingress.readRemote(yield);

  auto buf = array<uint8_t, 16>{};
  BOOST_CHECK_EQUAL(sticky.size(), ingress.recv(buf, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(sticky), cend(sticky), cbegin(buf),
                                cbegin(buf) + sticky.size());
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Tunnel_By_Insufficient_Buffer)
{
  auto content = "Very long content"sv;
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_HTTPS_HDR);
  socket.fill(END_HTTP_HDR);
  ingress.readRemote(yield);

  socket.fill(ConstBuffer<uint8_t>{content});

  auto buf = array<uint8_t, 16>{};
  for_each_n(begin(buf), content.size(), [&ingress](auto&& c) {
    BOOST_CHECK_EQUAL(1, ingress.recv({addressof(c), 1}, yield));
  });
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(content), cend(content), cbegin(buf),
                                cbegin(buf) + content.size());
  BOOST_CHECK_EXCEPTION(ingress.recv(buf, yield), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_Without_Connection_Field)
{
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_HTTP_HDR);
  socket.fill(HOST_FIELD);
  socket.fill(END_HTTP_HDR);
  ingress.readRemote(yield);

  auto buf = array<uint8_t, 1024>{};
  auto transfered = ingress.recv(buf, yield);

  auto parser = http::request_parser<http::empty_body>{};
  auto ec = sys::error_code{};

  BOOST_CHECK_EQUAL(transfered, parser.put(asio::buffer(buf), ec));
  BOOST_CHECK(!ec);
  BOOST_CHECK(parser.is_done());

  auto req = parser.release();
  BOOST_CHECK_EQUAL("/", req.target());

  verifyField(req, http::field::host, "localhost"sv);
  verifyField(req, http::field::connection, "close"sv);
  verifyField(req, http::field::proxy_connection, "close"sv);
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_With_Upgrade)
{
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_HTTP_HDR);
  socket.fill(HOST_FIELD);
  socket.fill(ConstBuffer<uint8_t>{"Connection: upgrade\r\n"sv});
  socket.fill(END_HTTP_HDR);
  ingress.readRemote(yield);

  auto buf = array<uint8_t, 1024>{};
  auto transfered = ingress.recv(buf, yield);

  auto parser = http::request_parser<http::empty_body>{};
  auto ec = sys::error_code{};

  BOOST_CHECK_EQUAL(transfered, parser.put(asio::buffer(buf), ec));
  BOOST_CHECK(!ec);
  BOOST_CHECK(parser.is_done());

  auto req = parser.release();
  BOOST_CHECK_EQUAL("/", req.target());

  verifyField(req, http::field::host, "localhost"sv);
  verifyField(req, http::field::connection, "upgrade"sv);
  BOOST_CHECK(req.find(http::field::proxy_connection) == cend(req));
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_With_Keep_Alive)
{
  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill(BEGIN_HTTP_HDR);
  socket.fill(HOST_FIELD);
  socket.fill(ConstBuffer<uint8_t>{"Connection: keep-alive\r\n"sv});
  socket.fill(END_HTTP_HDR);
  ingress.readRemote(yield);

  auto buf = array<uint8_t, 1024>{};
  auto transfered = ingress.recv(buf, yield);

  auto parser = http::request_parser<http::empty_body>{};
  auto ec = sys::error_code{};

  BOOST_CHECK_EQUAL(transfered, parser.put(asio::buffer(buf), ec));
  BOOST_CHECK(!ec);
  BOOST_CHECK(parser.is_done());

  auto req = parser.release();
  BOOST_CHECK_EQUAL("/", req.target());

  verifyField(req, http::field::host, "localhost"sv);
  verifyField(req, http::field::connection, "close"sv);
  verifyField(req, http::field::proxy_connection, "close"sv);
}

BOOST_AUTO_TEST_SUITE_END()
