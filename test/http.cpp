#define BOOST_TEST_MODULE pichi http test

#include "pichi/common/config.hpp"
#include "utils.hpp"
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <botan/base64.h>
#include <pichi/adapter/tcp/http.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/test.hpp>
#include <ranges>

using namespace std::literals;
namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace rngs  = std::ranges;
namespace sys   = boost::system;
namespace views = rngs::views;

namespace pichi::unit_test {

using Ingress = adapter::tcp::HttpIngress<TestSocket>;
using Egress  = adapter::tcp::HttpEgress<TestSocket>;

static auto const LOCALHOST = "localhost"sv;

static auto const LOCALHOST_URI = std::format("http://{}/", LOCALHOST);
static auto const ROOT_URI      = "/"sv;

static auto const HTTPS_ENDPOINT = makeEndpoint(LOCALHOST, 443);
static auto const HTTP_ENDPOINT  = makeEndpoint(LOCALHOST, 80);

static auto const HTTPS_TARGET = std::format("{}:{}", HTTPS_ENDPOINT.host_, HTTPS_ENDPOINT.port_);

static auto const CONTENT  = "content"s;
static auto const USERNAME = "pichi"s;
static auto const PASSWORD = "pichi"s;
static auto const IVO_AUTH =
    vo::Ingress{.credential_ = vo::UpIngressCredential{.credential_ = {{USERNAME, PASSWORD}}}};
static auto const EVO_AUTH =
    vo::Egress{.credential_ = vo::UpEgressCredential{.credential_ = {USERNAME, PASSWORD}}};

static auto const BASIC_FIELD      = "BASIC"sv;
static auto const CLOSE_FIELD      = "CLOSE"sv;
static auto const KEEP_ALIVE_FIELD = "KEEP-ALIVE"sv;
static auto const UPGRADE_FIELD    = "UPGRADE"sv;

static auto gen_request(http::verb method, std::string_view target)
{
  auto req = http::request<http::empty_body>{};
  req.method(method);
  req.target(target);
  req.version(11);
  req.set(http::field::host, LOCALHOST);
  return req;
}

static auto gen_request(std::string_view target, std::string_view body = CONTENT)
{
  auto req = http::request<http::string_body>{};
  req.method(http::verb::post);
  req.target(target);
  req.version(11);
  req.set(http::field::host, LOCALHOST);
  req.body() = body;
  req.prepare_payload();
  return req;
}

static auto gen_response(http::status code = http::status::no_content)
{
  auto resp = http::response<http::empty_body>{};
  resp.version(11);
  resp.result(code);
  return resp;
}

template <typename Body> static size_t serialize(http::response<Body> const& m, MutableBuffer buf)
{
  auto left = buf;
  auto sr   = http::response_serializer<Body>{m};
  while (!sr.is_done()) {
    auto ec = sys::error_code{};
    sr.next(ec, [&](auto& ec, auto&& serialized) {
      auto copied = asio::buffer_size(serialized);
      BOOST_REQUIRE_GE(rngs::size(left), copied);
      ec = {};
      rngs::copy_n(asio::buffers_begin(serialized), copied, rngs::begin(left));
      left += copied;
      sr.consume(copied);
    });
    BOOST_REQUIRE_EQUAL(sys::error_code{}, ec);
  }
  return rngs::size(buf) - rngs::size(left);
}

template <typename Data>
requires(std::constructible_from<ConstBuffer const&, Data>)
auto gen_auth(Data const& data)
{
  return std::format("{} {}", BASIC_FIELD, Botan::base64_encode(ConstBuffer{data}));
}

static auto gen_auth(std::string_view u, std::string_view p)
{
  return gen_auth(std::format("{}:{}", u, p));
}

template <bool isRequest, typename Body>
static http::message<isRequest, Body> parse(ConstBuffer buf)
{
  auto parser = http::parser<isRequest, Body>{};
  auto ec     = sys::error_code{};

  parser.eager(true);
  BOOST_CHECK_EQUAL(buf.size(), parser.put(asio::buffer(buf), ec));
  BOOST_CHECK(!ec);

  parser.put_eof(ec);
  BOOST_CHECK(!ec);

  BOOST_CHECK(parser.is_done());
  return parser.release();
}

static void verify_field(http::fields const& hdr, http::field field, std::string_view content)
{
  auto iequal = [](rngs::range auto&& lhs, rngs::range auto&& rhs) {
    return rngs::equal(lhs, rhs, [](auto a, auto b) { return tolower(a) == tolower(b); });
  };
  auto it = hdr.find(field);
  BOOST_CHECK(it != rngs::end(hdr));
  if (field == http::field::proxy_authorization) {
    auto v = ConstBuffer{it->value()};
    BOOST_CHECK(iequal(content | views::take(6), v | views::take(6)));
    BOOST_CHECK(rngs::equal(content | views::drop(6), v | views::drop(6)));
  }
  else {
    BOOST_CHECK(rngs::equal(content, it->value(), [](auto lhs, auto rhs) {
      return tolower(lhs) == tolower(rhs);
    }));
  }
}

BOOST_AUTO_TEST_SUITE(HTTP)

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Invalid_HTTP_Header)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await stream::write(client, "Not a legal http header");
    BOOST_CHECK_EXCEPTION(
        co_await ingress.read_remote(),
        SystemError,
        verify_exception<http::error::bad_version>
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Tunnel_Authentication_Without_Header)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto req = gen_request(http::verb::connect, HTTPS_TARGET);

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};

    co_await http::async_write(client, req, asio::use_awaitable);
    BOOST_CHECK_EXCEPTION(
        co_await ingress.read_remote(),
        SystemError,
        verify_exception<PichiError::BAD_AUTH_METHOD>
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authentication_Bad_Header)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto&& h : {
             ""s,                           // Empty header
             "Token XXXXX"s,                // Not basic
             "Basic "s,                     // Empty Credential
             "Basic invalid BASE64 code"s,  // Bad BASE64 sequence
         }) {
      for (auto req :
           {gen_request(http::verb::connect, HTTPS_TARGET),
            gen_request(http::verb::get, ROOT_URI)}) {
        req.set(http::field::proxy_authorization, h);

        auto client  = TestSocket{ex};
        auto ingress = Ingress{IVO_AUTH, client.peer()};

        co_await http::async_write(client, req, asio::use_awaitable);

        BOOST_CHECK_EXCEPTION(
            co_await ingress.read_remote(),
            SystemError,
            verify_exception<PichiError::BAD_AUTH_METHOD>
        );
      }
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authentication_Bad_Base64)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto&& h : {
             "Basic invalidBASE64code"s,  // Bad BASE64
             gen_auth("nocolon"),         // No colon
         }) {
      for (auto req :
           {gen_request(http::verb::connect, HTTPS_TARGET),
            gen_request(http::verb::get, ROOT_URI)}) {
        req.set(http::field::proxy_authorization, h);

        auto client  = TestSocket{ex};
        auto ingress = Ingress{IVO_AUTH, client.peer()};

        co_await http::async_write(client, req, asio::use_awaitable);

        BOOST_CHECK_EXCEPTION(
            co_await ingress.read_remote(),
            SystemError,
            verify_exception<PichiError::UNAUTHENTICATED>
        );
      }
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authentication_Bad_Credential)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto&& h : {
             ":bar"sv,  // Empty username
             "foo:"sv,  // Empty password
             ":"sv,     // Empty u&p
             "f:b"sv,   // Invalid u&p
         }) {
      for (auto req :
           {gen_request(http::verb::connect, HTTPS_TARGET),
            gen_request(http::verb::get, ROOT_URI)}) {
        req.set(http::field::proxy_authorization, gen_auth(h));

        auto client  = TestSocket{ex};
        auto ingress = Ingress{IVO_AUTH, client.peer()};

        co_await http::async_write(client, req, asio::use_awaitable);

        BOOST_CHECK_EXCEPTION(
            co_await ingress.read_remote(),
            SystemError,
            verify_exception<PichiError::UNAUTHENTICATED>
        );
      }
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authentication_Tunnel_Correct)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto req = gen_request(http::verb::connect, HTTPS_TARGET);
    req.set(http::field::proxy_authorization, gen_auth(USERNAME, PASSWORD));

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};

    co_await http::async_write(client, req, asio::use_awaitable);

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTPS_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.port_, remote.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Authentication_Relay_Correct)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto req = gen_request(http::verb::get, ROOT_URI);
    req.set(http::field::proxy_authorization, gen_auth(USERNAME, PASSWORD));

    auto client  = TestSocket{ex};
    auto ingress = Ingress{IVO_AUTH, client.peer()};

    co_await http::async_write(client, req, asio::use_awaitable);

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Tunnel_Correct)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::connect, HTTPS_TARGET),
        asio::use_awaitable
    );

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTPS_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.port_, remote.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Relay_With_Host_Field_Only)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(client, gen_request(http::verb::get, ROOT_URI), asio::use_awaitable);
    auto remote = co_await ingress.read_remote();

    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Relay_With_Both_Fields)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::get, LOCALHOST_URI),
        asio::use_awaitable
    );
    auto remote = co_await ingress.read_remote();

    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Relay_Without_Host_Field)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto req = gen_request(http::verb::get, LOCALHOST_URI);
    req.erase(http::field::host);

    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(client, req, asio::use_awaitable);
    auto remote = co_await ingress.read_remote();

    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Relay_With_Difference_Destinations)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};
    auto port    = 8080_u16;

    co_await http::async_write(
        client,
        gen_request(http::verb::get, std::format("http://{}:{}/", LOCALHOST, port)),
        asio::use_awaitable
    );
    auto remote = co_await ingress.read_remote();

    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(port, remote.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote_Relay_Without_Both_Fields)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto req = gen_request(http::verb::get, ROOT_URI);
    req.erase(http::field::host);

    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(client, req, asio::use_awaitable);

    BOOST_CHECK_EXCEPTION(
        co_await ingress.read_remote(),
        SystemError,
        verify_exception<PichiError::BAD_PROTO>
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Tunnel_With_Sticky_Content)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::connect, HTTPS_TARGET),
        asio::use_awaitable
    );
    co_await stream::write(client, CONTENT);

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTPS_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.port_, remote.port_);

    auto buf  = std::array<uint8_t, 1024>{};
    auto fact = buf | views::take(co_await ingress.recv(buf));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::cbegin(CONTENT),
        rngs::cend(CONTENT),
        rngs::cbegin(fact),
        rngs::cend(fact)
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Tunnel_By_Insufficient_Buffer)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::connect, HTTPS_TARGET),
        asio::use_awaitable
    );

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTPS_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.port_, remote.port_);

    co_await stream::write(client, CONTENT);

    auto buf = std::array<uint8_t, 1024>{};
    for (auto i : views::iota(0_sz, rngs::size(CONTENT))) {
      co_await ingress.recv(buf | views::drop(i) | views::take(1));
    };
    auto fact = buf | views::take(rngs::size(CONTENT));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CONTENT),
        rngs::end(CONTENT),
        rngs::begin(fact),
        rngs::end(fact)
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_Without_Connection_Field)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::get, LOCALHOST_URI),
        asio::use_awaitable
    );

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);

    auto buf = std::array<uint8_t, 1024>{};
    auto req = parse<true, http::empty_body>(buf | views::take(co_await ingress.recv(buf)));
    BOOST_CHECK_EQUAL(ROOT_URI, req.target());
    verify_field(req, http::field::host, LOCALHOST);
    verify_field(req, http::field::connection, CLOSE_FIELD);
    verify_field(req, http::field::proxy_connection, CLOSE_FIELD);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_With_Upgrade)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto origin = gen_request(http::verb::get, LOCALHOST_URI);
    origin.set(http::field::connection, UPGRADE_FIELD);

    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(client, origin, asio::use_awaitable);

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);

    auto buf = std::array<uint8_t, 1024>{};
    auto req = parse<true, http::empty_body>(buf | views::take(co_await ingress.recv(buf)));
    BOOST_CHECK_EQUAL(ROOT_URI, req.target());
    BOOST_CHECK(req.find(http::field::proxy_connection) == rngs::end(req));
    verify_field(req, http::field::host, LOCALHOST);
    verify_field(req, http::field::connection, UPGRADE_FIELD);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_With_Keep_Alive)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto origin = gen_request(http::verb::get, LOCALHOST_URI);
    origin.set(http::field::connection, KEEP_ALIVE_FIELD);

    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(client, origin, asio::use_awaitable);

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);

    auto buf = std::array<uint8_t, 1024>{};
    auto req = parse<true, http::empty_body>(buf | views::take(co_await ingress.recv(buf)));
    BOOST_CHECK_EQUAL(ROOT_URI, req.target());
    verify_field(req, http::field::host, LOCALHOST);
    verify_field(req, http::field::connection, CLOSE_FIELD);
    verify_field(req, http::field::proxy_connection, CLOSE_FIELD);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_With_Body)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(client, gen_request(LOCALHOST_URI), asio::use_awaitable);

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);

    auto buf = std::array<uint8_t, 1024>{};
    auto req = parse<true, http::string_body>(buf | views::take(co_await ingress.recv(buf)));

    BOOST_CHECK_EQUAL(ROOT_URI, req.target());
    BOOST_CHECK_EQUAL(CONTENT, req.body());
    verify_field(req, http::field::host, LOCALHOST);
    verify_field(req, http::field::connection, CLOSE_FIELD);
    verify_field(req, http::field::proxy_connection, CLOSE_FIELD);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv_Relay_By_Insufficient_Buffer)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(client, gen_request(LOCALHOST_URI), asio::use_awaitable);
    client.close();

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);

    auto buf  = std::array<uint8_t, 1024>{};
    auto len  = 0_sz;
    auto recv = [&]() -> Awaitable<void> {
      while (true) {
        co_await ingress.recv(buf | views::drop(len) | views::take(1));
        ++len;
      }
    };

    BOOST_CHECK_EXCEPTION(co_await recv(), SystemError, verify_exception<asio::error::eof>);

    auto req = parse<true, http::string_body>(buf | views::take(len));
    BOOST_CHECK_EQUAL(ROOT_URI, req.target());
    BOOST_CHECK_EQUAL(CONTENT, req.body());
    verify_field(req, http::field::host, LOCALHOST);
    verify_field(req, http::field::connection, CLOSE_FIELD);
    verify_field(req, http::field::proxy_connection, CLOSE_FIELD);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_confirm_Tunnel)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::connect, HTTPS_TARGET),
        asio::use_awaitable
    );

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTPS_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTPS_ENDPOINT.port_, remote.port_);

    co_await ingress.confirm();

    auto buf = std::array<uint8_t, 1024>{};
    auto rep =
        parse<false, http::empty_body>(buf | views::take(co_await stream::read_some(client, buf)));
    BOOST_CHECK_EQUAL(http::status::ok, rep.result());
  });
}

BOOST_AUTO_TEST_CASE(Ingress_confirm_Relay)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::get, LOCALHOST_URI),
        asio::use_awaitable
    );

    auto remote = co_await ingress.read_remote();
    BOOST_CHECK(HTTP_ENDPOINT.type_ == remote.type_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.host_, remote.host_);
    BOOST_CHECK_EQUAL(HTTP_ENDPOINT.port_, remote.port_);

    co_await ingress.confirm();
    co_await ingress.close();

    auto buf = std::array<uint8_t, 1024>{};
    BOOST_CHECK_EXCEPTION(
        co_await stream::read_some(client, buf),
        SystemError,
        verify_exception<asio::error::eof>
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect_For_PichiError)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto e :
         {PichiError::CONN_FAILURE,
          PichiError::BAD_AUTH_METHOD,
          PichiError::UNAUTHENTICATED,
          PichiError::BAD_PROTO,
          PichiError::CRYPTO_ERROR,
          PichiError::BUFFER_OVERFLOW,
          PichiError::BAD_JSON,
          PichiError::SEMANTIC_ERROR,
          PichiError::RES_IN_USE,
          PichiError::RES_LOCKED,
          PichiError::MISC}) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};

      co_await ingress.disconnect(make_error_code(e));
      co_await ingress.close();

      auto buf = std::array<uint8_t, 1024>{};
      auto len = co_await stream::read_some(client, buf);
      auto rep = parse<false, http::empty_body>(buf | views::take(len));
      switch (e) {
      case PichiError::CONN_FAILURE:
        BOOST_CHECK_EQUAL(http::status::gateway_timeout, rep.result());
        break;
      case PichiError::BAD_AUTH_METHOD:
        BOOST_CHECK_EQUAL(http::status::proxy_authentication_required, rep.result());
        verify_field(rep, http::field::proxy_authenticate, BASIC_FIELD);
        break;
      case PichiError::UNAUTHENTICATED:
        BOOST_CHECK_EQUAL(http::status::forbidden, rep.result());
        break;
      default:
        BOOST_CHECK_EQUAL(http::status::internal_server_error, rep.result());
        break;
      }
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect_For_HTTP_Errors)
{
  static auto const ECS = std::vector<sys::error_code>{
      http::error::bad_alloc,       http::error::bad_chunk,    http::error::bad_content_length,
      http::error::bad_method,      http::error::bad_obs_fold, http::error::bad_reason,
      http::error::bad_status,      http::error::bad_target,   http::error::bad_transfer_encoding,
      http::error::bad_value,       http::error::bad_version,  http::error::body_limit,
      http::error::buffer_overflow, http::error::end_of_chunk, http::error::end_of_stream,
      http::error::header_limit,    http::error::need_buffer,  http::error::need_more,
      http::error::partial_message, http::error::stale_parser, http::error::unexpected_body,
  };
  run_case([](auto&& ex) -> Awaitable<void> {
    for (auto e : ECS) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};

      co_await ingress.disconnect(e);
      co_await ingress.close();

      auto buf = std::array<uint8_t, 1024>{};
      auto len = co_await stream::read_some(client, buf);
      auto rep = parse<false, http::empty_body>(buf | views::take(len));
      BOOST_CHECK_EQUAL(http::status::bad_request, rep.result());
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect_For_Network_Errors)
{
  static auto const ECS = std::vector<sys::error_code>{
      asio::error::access_denied,
      asio::error::address_family_not_supported,
      asio::error::address_in_use,
      asio::error::already_connected,
      asio::error::bad_descriptor,
      asio::error::broken_pipe,
      asio::error::connection_aborted,
      asio::error::connection_refused,
      asio::error::connection_reset,
      asio::error::eof,
      asio::error::fault,
      asio::error::fd_set_failure,
      asio::error::host_not_found,
      asio::error::host_not_found_try_again,
      asio::error::host_unreachable,
      asio::error::in_progress,
      asio::error::interrupted,
      asio::error::invalid_argument,
      asio::error::message_size,
      asio::error::network_down,
      asio::error::network_reset,
      asio::error::network_unreachable,
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
    for (auto e : ECS) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};

      co_await ingress.disconnect(e);
      co_await ingress.close();

      auto buf = std::array<uint8_t, 1024>{};
      auto len = co_await stream::read_some(client, buf);
      auto rep = parse<false, http::empty_body>(buf | views::take(len));
      BOOST_CHECK_EQUAL(http::status::gateway_timeout, rep.result());
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_send_Tunnel)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::connect, HTTPS_TARGET),
        asio::use_awaitable
    );

    co_await ingress.read_remote();
    co_await ingress.confirm();

    auto buf = std::array<uint8_t, 1024>{};
    co_await stream::read_some(client, buf);

    co_await ingress.send(CONTENT);

    auto fact = buf | views::take(co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CONTENT),
        rngs::end(CONTENT),
        rngs::begin(fact),
        rngs::end(fact)
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_send_Relay)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::get, LOCALHOST_URI),
        asio::use_awaitable
    );

    co_await ingress.read_remote();
    co_await ingress.confirm();

    auto buf = std::array<uint8_t, 1024>{};
    co_await ingress.send(buf | views::take(serialize(gen_response(), buf)));

    auto rep =
        parse<false, http::empty_body>(buf | views::take(co_await stream::read_some(client, buf)));
    BOOST_CHECK_EQUAL(http::status::no_content, rep.result());
    verify_field(rep, http::field::connection, CLOSE_FIELD);
    verify_field(rep, http::field::proxy_connection, CLOSE_FIELD);

    co_await ingress.send(CONTENT);

    auto fact = buf | views::take(co_await stream::read_some(client, buf));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CONTENT),
        rngs::end(CONTENT),
        rngs::begin(fact),
        rngs::end(fact)
    );
  });
}

BOOST_AUTO_TEST_CASE(Ingress_send_Relay_With_Incomplete_Header)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto rep = std::array<uint8_t, 1024>{};
    auto len = serialize(gen_response(http::status::forbidden), rep);
    for (auto i = 0_sz; i < len; ++i) {
      auto client  = TestSocket{ex};
      auto ingress = Ingress{{}, client.peer()};

      co_await http::async_write(
          client,
          gen_request(http::verb::get, LOCALHOST_URI),
          asio::use_awaitable
      );

      co_await ingress.read_remote();
      co_await ingress.confirm();
      co_await ingress.send(rep | views::take(i));
      co_await ingress.close();

      auto buf = std::array<uint8_t, 1024>{};
      BOOST_CHECK_EXCEPTION(
          co_await stream::read_some(client, buf),
          SystemError,
          verify_exception<asio::error::eof>
      );
    }
  });
}

BOOST_AUTO_TEST_CASE(Ingress_send_Relay_One_By_One)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{{}, client.peer()};

    co_await http::async_write(
        client,
        gen_request(http::verb::get, LOCALHOST_URI),
        asio::use_awaitable
    );

    co_await ingress.read_remote();
    co_await ingress.confirm();

    auto buf = std::array<uint8_t, 1024>{};
    auto len = serialize(gen_response(), buf);
    for (auto i = 0_sz; i < len; ++i) co_await ingress.send(buf | views::drop(i) | views::take(1));

    auto rep =
        parse<false, http::empty_body>(buf | views::take(co_await stream::read_some(client, buf)));
    BOOST_CHECK_EQUAL(http::status::no_content, rep.result());
    verify_field(rep, http::field::connection, CLOSE_FIELD);
    verify_field(rep, http::field::proxy_connection, CLOSE_FIELD);

    for (auto i = 0_sz; i < rngs::size(CONTENT); ++i) {
      co_await ingress.send(CONTENT | views::drop(i) | views::take(1));
      BOOST_CHECK_EQUAL(1, co_await stream::read_some(client, buf));
      BOOST_CHECK_EQUAL(CONTENT[i], buf[0]);
    }
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Authentication)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto server = TestSocket{ex};
    auto egress = Egress{EVO_AUTH, server.peer()};

    auto buf = std::array<uint8_t, 1024>{};
    co_await stream::write(server, buf | views::take(serialize(gen_response(), buf)));

    co_await egress.connect(HTTPS_ENDPOINT);
    auto req =
        parse<true, http::empty_body>(buf | views::take(co_await stream::read_some(server, buf)));
    BOOST_CHECK_EQUAL(http::verb::connect, req.method());
    BOOST_CHECK_EQUAL(HTTPS_TARGET, req.target());
    verify_field(req, http::field::proxy_authorization, gen_auth(USERNAME, PASSWORD));
    verify_field(req, http::field::host, HTTPS_TARGET);
    verify_field(req, http::field::proxy_connection, KEEP_ALIVE_FIELD);

    co_await egress.close();
    BOOST_CHECK_EXCEPTION(
        co_await stream::read_some(server, buf),
        SystemError,
        verify_exception<asio::error::eof>
    );
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect_Without_Authentication)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto server = TestSocket{ex};
    auto egress = Egress{{}, server.peer()};

    auto buf = std::array<uint8_t, 1024>{};
    co_await stream::write(server, buf | views::take(serialize(gen_response(), buf)));

    co_await egress.connect(HTTPS_ENDPOINT);
    auto req =
        parse<true, http::empty_body>(buf | views::take(co_await stream::read_some(server, buf)));
    BOOST_CHECK_EQUAL(http::verb::connect, req.method());
    BOOST_CHECK_EQUAL(HTTPS_TARGET, req.target());
    verify_field(req, http::field::host, HTTPS_TARGET);
    verify_field(req, http::field::proxy_connection, KEEP_ALIVE_FIELD);

    co_await egress.close();
    BOOST_CHECK_EXCEPTION(
        co_await stream::read_some(server, buf),
        SystemError,
        verify_exception<asio::error::eof>
    );
  });
}

BOOST_AUTO_TEST_CASE(Egress_send_Content)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto server = TestSocket{ex};
    auto egress = Egress{{}, server.peer()};

    auto buf = std::array<uint8_t, 1024>{};
    co_await stream::write(server, buf | views::take(serialize(gen_response(), buf)));

    co_await egress.connect(HTTPS_ENDPOINT);
    co_await stream::read_some(server, buf);

    co_await egress.send(CONTENT);
    co_await egress.close();

    auto fact = buf | views::take(co_await stream::read_some(server, buf));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CONTENT),
        rngs::end(CONTENT),
        rngs::begin(fact),
        rngs::end(fact)
    );
    BOOST_CHECK_EXCEPTION(
        co_await stream::read_some(server, buf),
        SystemError,
        verify_exception<asio::error::eof>
    );
  });
}

BOOST_AUTO_TEST_CASE(Egress_recv_Content)
{
  run_case([](auto&& ex) -> Awaitable<void> {
    auto server = TestSocket{ex};
    auto egress = Egress{{}, server.peer()};

    auto buf = std::array<uint8_t, 1024>{};
    co_await stream::write(server, buf | views::take(serialize(gen_response(), buf)));

    co_await egress.connect(HTTPS_ENDPOINT);
    co_await stream::read_some(server, buf);

    co_await stream::write(server, CONTENT);
    co_await stream::close(server);

    auto fact = buf | views::take(co_await egress.recv(buf));
    BOOST_CHECK_EQUAL_COLLECTIONS(
        rngs::begin(CONTENT),
        rngs::end(CONTENT),
        rngs::begin(fact),
        rngs::end(fact)
    );
    BOOST_CHECK_EXCEPTION(
        co_await egress.recv(buf),
        SystemError,
        verify_exception<asio::error::eof>
    );
  });
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
