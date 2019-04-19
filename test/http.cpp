#define BOOST_TEST_MODULE pichi http test

#include "config.h"
#include "utils.hpp"
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/net/asio.hpp>
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

static auto genTunnelReq()
{
  auto req = http::request<http::empty_body>{};
  req.method(http::verb::connect);
  req.target("localhost:443");
  req.version(11);
  return req;
}

static auto genOrignalRelayReq()
{
  auto req = http::request<http::empty_body>{};
  req.method(http::verb::get);
  req.target("http://localhost/");
  req.version(11);
  req.set(http::field::host, "localhost");
  return req;
}

static auto genParsedRelayReq()
{
  auto req = http::request<http::empty_body>{};
  req.method(http::verb::get);
  req.target("/");
  req.version(11);
  req.set(http::field::host, "localhost");
  return req;
}

static auto genRelayReqWithBody()
{
  auto req = http::request<http::string_body>{};
  req.method(http::verb::post);
  req.target("http://localhost/");
  req.version(11);
  req.set(http::field::host, "localhost");
  req.body() = "http body";
  req.prepare_payload();
  return req;
}

static auto genResponse()
{
  auto resp = http::response<http::empty_body>{};
  resp.version(11);
  resp.result(http::status::no_content);
  return resp;
}

template <bool isRequest, typename Body>
static size_t serializeToBuffer(http::message<isRequest, Body>& m, MutableBuffer<uint8_t> buf)
{
  auto left = buf;
  auto sr = http::serializer<isRequest, Body>{m};
  while (!sr.is_done()) {
    auto ec = sys::error_code{};
    sr.next(ec, [&](auto& ec, auto&& serialized) {
      auto copied = asio::buffer_size(serialized);
      BOOST_REQUIRE_GE(left.size(), copied);
      ec = {};
      copy_n(asio::buffers_begin(serialized), copied, begin(left));
      left += copied;
      sr.consume(copied);
    });
    BOOST_REQUIRE_EQUAL(sys::error_code{}, ec);
  }
  return buf.size() - left.size();
}

template <bool isRequest, typename Body>
static http::message<isRequest, Body> parseFromBuffer(ConstBuffer<uint8_t> buf)
{
  auto parser = http::parser<isRequest, Body>{};
  auto ec = sys::error_code{};

  parser.eager(true);
  BOOST_CHECK_EQUAL(buf.size(), parser.put(asio::buffer(buf), ec));
  BOOST_CHECK(!ec);

  parser.put_eof(ec);
  BOOST_CHECK(!ec);

  BOOST_CHECK(parser.is_done());
  return parser.release();
}

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
  auto buf = array<uint8_t, 64>{};
  auto req = genTunnelReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(req, buf)});
  auto remote = ingress.readRemote(yield);

  BOOST_CHECK(HTTPS_ENDPOINT.type_ == remote.type_);
  BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.host_, remote.host_);
  BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.port_, remote.port_);
  BOOST_CHECK_EXCEPTION(ingress.recv(buf, yield), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(readRemote_Relay_With_Host_Field_Only)
{
  auto buf = array<uint8_t, 64>{};
  auto req = genParsedRelayReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(req, buf)});
  auto remote = ingress.readRemote(yield);

  BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
  BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
  BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);
}

BOOST_AUTO_TEST_CASE(readRemote_Relay_With_Both_Fields)
{
  auto buf = array<uint8_t, 64>{};
  auto req = genOrignalRelayReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(req, buf)});
  auto remote = ingress.readRemote(yield);

  BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
  BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
  BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);
}

BOOST_AUTO_TEST_CASE(readRemote_Relay_Without_Host_Field)
{
  auto buf = array<uint8_t, 64>{};
  auto req = genOrignalRelayReq();
  req.erase(http::field::host);

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(req, buf)});

  BOOST_CHECK_EXCEPTION(ingress.readRemote(yield), Exception,
                        verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Tunnel_With_Sticky_Content)
{
  auto buf = array<uint8_t, 64>{};
  auto req = genTunnelReq();
  auto sticky = "sticky content"sv;

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(req, buf)});
  socket.fill(ConstBuffer<uint8_t>{sticky});

  ingress.readRemote(yield);

  BOOST_CHECK_EQUAL(sticky.size(), ingress.recv(buf, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(sticky), cend(sticky), cbegin(buf),
                                cbegin(buf) + sticky.size());
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Tunnel_By_Insufficient_Buffer)
{
  auto buf = array<uint8_t, 64>{};
  auto req = genTunnelReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(req, buf)});
  ingress.readRemote(yield);

  auto content = "Very long content"sv;
  socket.fill(ConstBuffer<uint8_t>{content});

  for (auto i = 0; i < content.size(); ++i)
    BOOST_CHECK_EQUAL(1, ingress.recv({buf.data() + i, 1}, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(content), cend(content), cbegin(buf),
                                cbegin(buf) + content.size());
  BOOST_CHECK_EXCEPTION(ingress.recv(buf, yield), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_Without_Connection_Field)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genOrignalRelayReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});
  ingress.readRemote(yield);

  auto received = parseFromBuffer<true, http::empty_body>({buf, ingress.recv(buf, yield)});
  BOOST_CHECK_EQUAL("/", received.target());

  verifyField(received, http::field::host, "localhost"sv);
  verifyField(received, http::field::connection, "close"sv);
  verifyField(received, http::field::proxy_connection, "close"sv);
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_With_Upgrade)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genOrignalRelayReq();
  origin.set(http::field::connection, "upgrade");

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});
  ingress.readRemote(yield);

  auto received = parseFromBuffer<true, http::empty_body>({buf, ingress.recv(buf, yield)});
  BOOST_CHECK_EQUAL("/", received.target());

  verifyField(received, http::field::host, "localhost"sv);
  verifyField(received, http::field::connection, "upgrade"sv);
  BOOST_CHECK(received.find(http::field::proxy_connection) == cend(received));
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_With_Keep_Alive)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genOrignalRelayReq();
  origin.set(http::field::connection, "keey-alive");

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});
  ingress.readRemote(yield);

  auto req = parseFromBuffer<true, http::empty_body>({buf, ingress.recv(buf, yield)});
  BOOST_CHECK_EQUAL("/", req.target());

  verifyField(req, http::field::host, "localhost"sv);
  verifyField(req, http::field::connection, "close"sv);
  verifyField(req, http::field::proxy_connection, "close"sv);
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_With_Body)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genRelayReqWithBody();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});
  ingress.readRemote(yield);

  // Read header on the first ingress.recv, and body on the second ingress.recv
  auto transfered = ingress.recv(buf, yield);
  transfered += ingress.recv(MutableBuffer<uint8_t>{buf} + transfered, yield);
  auto req = parseFromBuffer<true, http::string_body>({buf, transfered});
  BOOST_CHECK_EQUAL("/", req.target());

  verifyField(req, http::field::host, "localhost"sv);
  verifyField(req, http::field::connection, "close"sv);
  verifyField(req, http::field::proxy_connection, "close"sv);
  BOOST_CHECK_EQUAL("http body", req.body());
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_By_Insufficient_Buffer)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genRelayReqWithBody();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});
  ingress.readRemote(yield);

  // Invoke ingress.recv until socket is empty
  auto data = MutableBuffer<uint8_t>{buf};
  auto recv = [&]() {
    while (true) {
      BOOST_CHECK_EQUAL(1, ingress.recv({data, 1}, yield));
      data += 1;
    }
  };
  BOOST_CHECK_EXCEPTION(recv(), Exception, verifyException<PichiError::MISC>);

  auto received = parseFromBuffer<true, http::string_body>({buf, buf.size() - data.size()});
  BOOST_CHECK_EQUAL("/", received.target());

  verifyField(received, http::field::host, "localhost"sv);
  verifyField(received, http::field::connection, "close"sv);
  verifyField(received, http::field::proxy_connection, "close"sv);
  BOOST_CHECK_EQUAL("http body", received.body());
}

BOOST_AUTO_TEST_CASE(Ingress_confirm_Tunnel)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genTunnelReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});

  ingress.readRemote(yield);
  ingress.confirm(yield);

  auto confirmed = parseFromBuffer<false, http::empty_body>({buf, socket.flush(buf)});
  BOOST_CHECK_EQUAL(http::status::ok, confirmed.result());
}

BOOST_AUTO_TEST_CASE(Ingress_confirm_Relay)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genOrignalRelayReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});

  ingress.readRemote(yield);
  ingress.confirm(yield);

  BOOST_CHECK_EQUAL(0, socket.available());
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect_Tunnel)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genTunnelReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});

  ingress.readRemote(yield);
  ingress.disconnect(yield);

  auto disconnected = parseFromBuffer<false, http::empty_body>({buf, socket.flush(buf)});
  BOOST_CHECK_EQUAL(http::status::gateway_timeout, disconnected.result());
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect_Relay)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genOrignalRelayReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(origin, buf)});

  ingress.readRemote(yield);
  ingress.disconnect(yield);

  auto disconnected = parseFromBuffer<false, http::empty_body>({buf, socket.flush(buf)});
  BOOST_CHECK_EQUAL(http::status::gateway_timeout, disconnected.result());
}

BOOST_AUTO_TEST_CASE(Ingress_send_Tunnel)
{
  auto buf = array<uint8_t, 1024>{};
  auto origin = genTunnelReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  // Handshake
  socket.fill({buf, serializeToBuffer(origin, buf)});
  ingress.readRemote(yield);
  ingress.confirm(yield);
  socket.flush(buf);

  auto content = "content"sv;

  ingress.send(ConstBuffer<uint8_t>{content}, yield);
  BOOST_CHECK_EQUAL(content.size(), socket.available());

  socket.flush({buf, content.size()});
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(content), cend(content), cbegin(buf),
                                cbegin(buf) + content.size());
}

BOOST_AUTO_TEST_CASE(Ingress_send_Relay)
{
  auto buf = array<uint8_t, 1024>{};
  auto req = genOrignalRelayReq();

  auto socket = Socket{};
  auto ingress = HttpIngress{socket, true};

  socket.fill({buf, serializeToBuffer(req, buf)});
  ingress.readRemote(yield);
  ingress.confirm(yield);
  BOOST_CHECK_EQUAL(0, socket.available());

  auto origin = genResponse();
  ingress.send({buf, serializeToBuffer(origin, buf)}, yield);

  auto sent = parseFromBuffer<false, http::empty_body>({buf, socket.flush(buf)});
  BOOST_CHECK_EQUAL(http::status::no_content, sent.result());
  verifyField(sent, http::field::connection, "close"sv);
  verifyField(sent, http::field::proxy_connection, "close"sv);
}

BOOST_AUTO_TEST_SUITE_END()
