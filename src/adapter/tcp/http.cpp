#include "pichi/common/config.hpp"
#include <algorithm>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/system/system_error.hpp>
#include <botan/base64.h>
#include <format>
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/adapter/tcp/http.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/uri.hpp>
#include <pichi/stream/helpers.hpp>
#include <pichi/stream/test.hpp>
#include <pichi/stream/tls.hpp>
#include <ranges>
#include <regex>

namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace rngs  = std::ranges;
namespace sys   = boost::system;
namespace views = rngs::views;

using namespace std::literals;

namespace pichi::adapter::tcp {

namespace detail {

// FIXME hardcode HTTP header limit to 1M
static uint32_t const HEADER_LIMIT = 1024ul * 1024ul;

static auto const BASIC_AUTH_PATTERN = std::regex{"^basic ([a-z0-9+/]+={0,2})", std::regex::icase};

static Endpoint get_remote(Request& req)
{
  /*
   * HTTP Proxy @RFC2068
   *   The HOST field and absolute_path are both mandatory and same according
   * to the standard. But some non-standard clients might send request:
   *     - with different destinations discribed in HOST field and
   * absolute_path;
   *     - without absolute_path but relative_path specified;
   *     - without HOST field but absolute_path specified.
   *   The rules, which are not very strict but still standard, listed below
   * are followed to handle these non-standard clients:
   *     - the destination described in absolute_path would be chosen;
   *     - the destination described in HOST field would be chosen if
   * relative_path specified;
   *     - relative_path will be forwarded without any change.
   */
  auto target = std::string{std::cbegin(req.target()), std::cend(req.target())};
  assertFalse(target.empty(), PichiError::BAD_PROTO, "Empty path");
  if (target[0] != '/') {
    // absolute_path specified, so convert it to relative one.
    auto uri = Uri{target};
    req.target(boost::string_view{uri.suffix_.data(), uri.suffix_.size()});
    return makeEndpoint(uri.host_, uri.port_);
  }
  else {
    // HOST field is mandatory now since relative_path specified.
    auto it = req.find(http::field::host);
    assertTrue(it != std::cend(req), PichiError::BAD_PROTO, "Lack of target information");
    auto hp = HostAndPort{
        {it->value().data(), it->value().size()}
    };
    return makeEndpoint(hp.host_, hp.port_);
  }
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> InvalidManner<NextLayer>::recv(NextLayer&, MutableBuffer)
{
  fail("Unexpected invocation");
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> InvalidManner<NextLayer>::send(NextLayer&, ConstBuffer)
{
  fail("Unexpected invocation");
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> InvalidManner<NextLayer>::confirm(NextLayer&)
{
  fail("Unexpected invocation");
}

template <stream::AsyncLayer NextLayer>
ConnectManner<NextLayer>::ConnectManner(Cache cache) : cache_{cache}
{
}
template <stream::AsyncLayer NextLayer>
Awaitable<size_t> ConnectManner<NextLayer>::recv(NextLayer& underlying, MutableBuffer buf)
{
  if (cache_.size() == 0) co_return co_await stream::read_some(underlying, buf);

  auto copied = std::min(buf.size(), cache_.size());
  std::copy_n(asio::buffers_begin(cache_.data()), copied, std::begin(buf));
  cache_.consume(copied);
  co_return copied;
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> ConnectManner<NextLayer>::send(NextLayer& underlying, ConstBuffer buf)
{
  co_await stream::write(underlying, buf);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> ConnectManner<NextLayer>::confirm(NextLayer& underlying)
{
  auto rep = Response{};
  rep.version(11);
  rep.result(http::status::ok);
  rep.reason("Connection Established");
  co_await http::async_write(underlying, rep, asio::use_awaitable);
}

template <stream::AsyncLayer NextLayer>
ProxyManner<NextLayer>::ProxyManner(Cache cache, Request req)
  : in_{std::move(cache)}, out_{}, parser_{}
{
  auto sticked = in_.size();

  auto sr = http::serializer<true, Body>{req};
  do {
    auto ec = sys::error_code{};
    sr.next(ec, [&](auto&&, auto serialized) {
      auto n = asio::buffer_size(serialized);
      asio::buffer_copy(in_.prepare(n), serialized);
      in_.commit(n);
      sr.consume(n);
    });
    asio::detail::throw_error(ec);
  } while (!sr.is_header_done());

  asio::buffer_copy(in_.prepare(sticked), in_.data());
  in_.commit(sticked);
  in_.consume(sticked);
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> ProxyManner<NextLayer>::recv(NextLayer& underlying, MutableBuffer buf)
{
  if (in_.size() == 0) co_return co_await stream::read_some(underlying, buf);

  auto copied = std::min(in_.size(), buf.size());
  std::copy_n(asio::buffers_begin(in_.data()), copied, std::begin(buf));
  in_.consume(copied);
  co_return copied;
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> ProxyManner<NextLayer>::send(NextLayer& underlying, ConstBuffer buf)
{
  if (parser_.is_header_done()) {
    co_await stream::write(underlying, buf);
    co_return;
  }

  if (out_.size() > 0) {
    auto n = rngs::size(buf);
    rngs::copy(buf, asio::buffers_begin(out_.prepare(n)));
    out_.commit(n);
    buf = out_.cdata();
  }

  auto ec = sys::error_code{};
  auto n  = parser_.put(asio::buffer(buf), ec);
  if (ec && ec != http::error::need_more) throw sys::system_error(ec);

  if (out_.size() > 0) {
    out_.consume(n);
  }
  else {
    rngs::copy(buf | views::drop(n), asio::buffers_begin(out_.prepare(rngs::size(buf) - n)));
    out_.commit(rngs::size(buf) - n);
  }

  if (!parser_.is_header_done()) co_return;

  auto rep = parser_.release();
  if (!parser_.upgrade()) {
    rep.set(http::field::connection, "close");
    rep.set(http::field::proxy_connection, "close");
  }

  co_await http::async_write(underlying, rep, asio::use_awaitable);
  if (out_.size() > 0) {
    co_await stream::write(underlying, out_.cdata());
    out_.consume(out_.size());
  }
}

template <stream::AsyncLayer NextLayer> Awaitable<void> ProxyManner<NextLayer>::confirm(NextLayer&)
{
  co_return;
}

static auto gen_auth(std::string_view u, std::string_view p)
{
  return Botan::base64_encode(ConstBuffer{std::format("{}:{}", u, p)});
}

static auto gen_credentials(vo::Ingress const& vo)
{
  auto ret = std::unordered_set<std::string>{};
  if (vo.credential_.has_value()) {
    for (auto&& item : std::get<vo::UpIngressCredential>(*vo.credential_).credential_) {
      ret.emplace(gen_auth(item.first, item.second));
    }
  }
  return ret;
}

static bool
    authenticate(http::fields const& req, std::unordered_set<std::string> const& credentials)
{
  auto it = req.find(http::field::proxy_authorization);
  assertFalse(it == std::cend(req), PichiError::BAD_AUTH_METHOD);

  auto match = std::cmatch{};
  auto auth  = it->value();
  assertTrue(
      std::regex_match(std::cbegin(auth), std::cend(auth), match, BASIC_AUTH_PATTERN),
      PichiError::BAD_AUTH_METHOD
  );
  assertTrue(2 == match.size() && match[1].matched, PichiError::BAD_AUTH_METHOD);

  return credentials.contains({match[1].first, static_cast<size_t>(match[1].length())});
}

static auto gen_credential(vo::Egress const& vo)
{
  if (!vo.credential_.has_value()) return ""s;

  auto&& pair = std::get<vo::UpEgressCredential>(*vo.credential_).credential_;
  return std::format("Basic {}", gen_auth(pair.first, pair.second));
}

}  // namespace detail

template <stream::AsyncLayer NextLayer>
HttpIngress<NextLayer>::HttpIngress(vo::Ingress const& vo, NextLayer underlying)
  : underlying_{std::move(underlying)},
    manner_{detail::InvalidManner<NextLayer>{}},
    credentials_{detail::gen_credentials(vo)}
{
  parser_.header_limit(detail::HEADER_LIMIT);
  parser_.body_limit(std::numeric_limits<uint64_t>::max());
}

template <stream::AsyncLayer NextLayer> Awaitable<void> HttpIngress<NextLayer>::close()
{
  co_await redirect(stream::close(underlying_));
}

template <stream::AsyncLayer NextLayer> Awaitable<Endpoint> HttpIngress<NextLayer>::read_remote()
{
  co_await stream::accept(underlying_);

  co_await http::async_read_header(underlying_, cache_, parser_, asio::use_awaitable);

  auto req = parser_.release();

  if (!rngs::empty(credentials_))
    assertTrue(detail::authenticate(req, credentials_), PichiError::UNAUTHENTICATED);

  if (req.method() == http::verb::connect) {
    manner_.template emplace<detail::ConnectManner<NextLayer>>(std::move(cache_));

    /*
     * HTTP CONNECT @RFC2616
     *   Don't validate whether the HOST field exists or not here.
     *   Some clients are not standard and send the CONNECT request without HOST
     * field.
     */
    auto hp = HostAndPort{
        {req.target().data(), req.target().size()}
    };
    co_return makeEndpoint(hp.host_, hp.port_);
  }
  else {
    auto remote = detail::get_remote(req);
    if (!parser_.upgrade()) {
      req.set(http::field::connection, "close");
      req.set(http::field::proxy_connection, "close");
    }
    manner_.template emplace<detail::ProxyManner<NextLayer>>(std::move(cache_), std::move(req));
    co_return remote;
  }
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> HttpIngress<NextLayer>::recv(MutableBuffer buf)
{
  co_return co_await std::visit(
      [buf, this](auto&& m) { return m.recv(underlying_, buf); },
      manner_
  );
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> HttpIngress<NextLayer>::send(ConstBuffer buf)
{
  co_return co_await std::visit(
      [buf, this](auto&& m) { return m.send(underlying_, buf); },
      manner_
  );
}

template <stream::AsyncLayer NextLayer> Awaitable<void> HttpIngress<NextLayer>::confirm()
{
  co_return co_await std::visit([this](auto&& m) { return m.confirm(underlying_); }, manner_);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> HttpIngress<NextLayer>::disconnect(sys::error_code const& ec)
{
  if (!ec) co_return;

  auto rep = detail::Response{};
  rep.version(11);
  rep.set(http::field::connection, "Close");
  if (ec == PichiError::CONN_FAILURE) {
    rep.result(http::status::gateway_timeout);
  }
  else if (ec == PichiError::BAD_AUTH_METHOD) {
    rep.result(http::status::proxy_authentication_required);
    rep.set(http::field::proxy_authenticate, "Basic");
  }
  else if (ec == PichiError::UNAUTHENTICATED) {
    rep.result(http::status::forbidden);
  }
  else if (ec.category() == PICHI_CATEGORY) {
    rep.result(http::status::internal_server_error);
  }
  else {
    /* TODO It's not clear that http::make_error_code() will return error codes
     * with different categories when its invoker are located in the different
     * DLLs. As the result, the pre-defined category will never equal to what
     * comes from Boost.Test. The downcasting at runtime has to be used to
     * detect the equation here.
     *
     * The original code:
     * static auto const& HTTP_ERROR_CATEGORY =
     *   http::make_error_code(http::error::end_of_stream).category();
     */

    using CategoryPtr = http::detail::http_error_category const*;
    auto pCat         = dynamic_cast<CategoryPtr>(&ec.category());
    rep.result(pCat ? http::status::bad_request : http::status::gateway_timeout);
  }
  co_await http::async_write(underlying_, rep, asio::use_awaitable);
}

template class HttpIngress<Socket>;
template class HttpIngress<Tls>;
template class HttpIngress<unit_test::TestSocket>;

template <stream::AsyncLayer NextLayer>
HttpEgress<NextLayer>::HttpEgress(vo::Egress const& vo, NextLayer underlying)
  : underlying_{std::move(underlying)}, peer_{*vo.server_}, credential_{detail::gen_credential(vo)}
{
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> HttpEgress<NextLayer>::recv(MutableBuffer buf)
{
  if (cache_.size() == 0) co_return co_await stream::read_some(underlying_, buf);
  auto copied = std::min(cache_.size(), buf.size());
  std::copy_n(asio::buffers_begin(cache_.data()), copied, std::begin(buf));
  cache_.consume(copied);
  co_return copied;
}

template <stream::AsyncLayer NextLayer> Awaitable<void> HttpEgress<NextLayer>::send(ConstBuffer buf)
{
  co_await stream::write(underlying_, buf);
}

template <stream::AsyncLayer NextLayer> Awaitable<void> HttpEgress<NextLayer>::close()
{
  co_await redirect(stream::close(underlying_));
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> HttpEgress<NextLayer>::connect(Endpoint const& remote)
{
  co_await stream::connect(underlying_, peer_);

  auto req = detail::Request{};
  req.method(http::verb::connect);
  if (remote.type_ == EndpointType::IPV6)
    req.target(std::format("[{}:{}]", remote.host_, remote.port_));
  else
    req.target(std::format("{}:{}", remote.host_, remote.port_));
  req.set(http::field::host, req.target());
  req.set(http::field::proxy_connection, "Keep-Alive");
  if (!rngs::empty(credential_)) req.set(http::field::proxy_authorization, credential_);
  req.prepare_payload();

  co_await http::async_write(underlying_, req, asio::use_awaitable);

  auto parser = detail::ResponseParser{};
  co_await http::async_read_header(underlying_, cache_, parser, asio::use_awaitable);
  auto code = parser.release().result_int();
  assertTrue(code >= 200 && code < 300, PichiError::CONN_FAILURE);
}

template class HttpEgress<Socket>;
template class HttpEgress<Tls>;
template class HttpEgress<unit_test::TestSocket>;

}  // namespace pichi::adapter::tcp
