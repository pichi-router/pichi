#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <format>
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/adapter/tcp/http.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/uri.hpp>
#include <pichi/crypto/base64.hpp>
#include <pichi/stream/helpers.hpp>
#include <pichi/stream/tls.hpp>
#include <regex>

namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace sys  = boost::system;

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

template <typename Stream> Awaitable<size_t> InvalidManner::recv(Stream&, MutableBuffer)
{
  fail("Unexpected invocation");
}

template <typename Stream> Awaitable<void> InvalidManner::send(Stream&, ConstBuffer)
{
  fail("Unexpected invocation");
}

template <typename Stream> Awaitable<void> InvalidManner::confirm(Stream&)
{
  fail("Unexpected invocation");
}

ConnectManner::ConnectManner(Cache cache) : cache_{cache} {}

template <typename Stream> Awaitable<size_t> ConnectManner::recv(Stream& stream, MutableBuffer buf)
{
  if (cache_.size() == 0) co_return co_await stream::read_some(stream, buf);

  auto copied = std::min(buf.size(), cache_.size());
  std::copy_n(asio::buffers_begin(cache_.data()), copied, std::begin(buf));
  cache_.consume(copied);
  co_return copied;
}

template <typename Stream> Awaitable<void> ConnectManner::send(Stream& stream, ConstBuffer buf)
{
  co_await stream::write(stream, buf);
}

template <typename Stream> Awaitable<void> ConnectManner::confirm(Stream& stream)
{
  auto rep = Response{};
  rep.version(11);
  rep.result(http::status::ok);
  rep.reason("Connection Established");
  co_await http::async_write(stream, rep);
}

ProxyManner::ProxyManner(Cache cache, Request req) : cache_{std::move(cache)}
{
  auto sticked = cache_.size();

  auto sr = http::serializer<true, Body>{req};
  do {
    auto ec = sys::error_code{};
    sr.next(ec, [&](auto&&, auto serialized) {
      auto n = asio::buffer_size(serialized);
      asio::buffer_copy(cache_.prepare(n), serialized);
      cache_.commit(n);
      sr.consume(n);
    });
    asio::detail::throw_error(ec);
  } while (!sr.is_header_done());

  asio::buffer_copy(cache_.prepare(sticked), cache_.data());
  cache_.consume(sticked);
}

template <typename Stream> Awaitable<size_t> ProxyManner::recv(Stream& stream, MutableBuffer buf)
{
  if (cache_.size() == 0) co_return co_await stream::read_some(stream, buf);

  auto copied = std::min(cache_.size(), buf.size());
  std::copy_n(asio::buffers_begin(cache_.data()), copied, std::begin(buf));
  cache_.consume(copied);
  co_return copied;
}

template <typename Stream> Awaitable<void> ProxyManner::send(Stream& stream, ConstBuffer buf)
{
  co_await stream::write(stream, buf);
}

template <typename Stream> Awaitable<void> ProxyManner::confirm(Stream&) { co_return; }

static auto get_credential(Request const& req)
{
  auto auth = req.find(http::field::proxy_authorization);
  assertFalse(auth == std::cend(req), PichiError::BAD_AUTH_METHOD);
  auto m          = std::cmatch{};
  auto credential = auth->value();
  assertTrue(
      std::regex_match(std::cbegin(credential), std::cend(credential), m, BASIC_AUTH_PATTERN),
      PichiError::BAD_AUTH_METHOD
  );
  assertTrue(2 == m.size() && m[1].matched, PichiError::BAD_AUTH_METHOD);
  auto plain = crypto::base64Decode(
      {m[1].first, static_cast<size_t>(m[1].length())},
      PichiError::BAD_AUTH_METHOD
  );
  auto colon = plain.find_first_of(':');
  assertFalse(colon == std::string::npos, PichiError::BAD_AUTH_METHOD);
  return std::make_pair(plain.substr(0, colon), plain.substr(colon + 1));
}

HttpIngressCredential::HttpIngressCredential(vo::Ingress const& vo)
  : data_{
        vo.credential_.has_value() ? std::get<vo::UpIngressCredential>(*vo.credential_).credential_
                                   : std::unordered_map<std::string, std::string>{}
    }
{
}

bool HttpIngressCredential::authenticate(Request const& req) const
{
  if (data_.empty()) return true;

  auto [u, p] = get_credential(req);
  auto it     = data_.find(u);
  return it != std::end(data_) && it->second == p;
}

HttpEgressCredential::HttpEgressCredential(vo::Egress const& vo)
{
  if (!vo.credential_.has_value()) return;

  auto&& pair = std::get<vo::UpEgressCredential>(*vo.credential_).credential_;
  data_ =
      std::format("Basic {}", crypto::base64Encode(std::format("{}:{}", pair.first, pair.second)));
}

void HttpEgressCredential::update(Request& req) const
{
  if (data_.empty()) return;
  req.set(http::field::proxy_authorization, data_);
}

}  // namespace detail

template <typename Stream>
HttpIngress<Stream>::HttpIngress(vo::Ingress const& vo, Stream&& stream)
  : stream_(std::move(stream)), credential_{vo}
{
  parser_.header_limit(detail::HEADER_LIMIT);
  parser_.body_limit(std::numeric_limits<uint64_t>::max());
}

template <typename Stream> Awaitable<void> HttpIngress<Stream>::close()
{
  co_await redirect(stream::close(stream_));
}

template <typename Stream> Awaitable<Endpoint> HttpIngress<Stream>::read_remote()
{
  co_await stream::accept(stream_);

  co_await http::async_read_header(stream_, cache_, parser_);

  auto req = parser_.release();

  assertTrue(credential_.authenticate(req), PichiError::UNAUTHENTICATED);

  if (req.method() == http::verb::connect) {
    manner_.template emplace<detail::ConnectManner>(std::move(cache_));

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
    manner_.template emplace<detail::ProxyManner>(std::move(cache_), std::move(req));
    co_return remote;
  }
}

template <typename Stream> Awaitable<size_t> HttpIngress<Stream>::recv(MutableBuffer buf)
{
  co_return co_await std::visit([buf, this](auto&& m) { return m.recv(stream_, buf); }, manner_);
}

template <typename Stream> Awaitable<void> HttpIngress<Stream>::send(ConstBuffer buf)
{
  co_return co_await std::visit([buf, this](auto&& m) { return m.send(stream_, buf); }, manner_);
}

template <typename Stream> Awaitable<void> HttpIngress<Stream>::confirm()
{
  co_return co_await std::visit([this](auto&& m) { return m.confirm(stream_); }, manner_);
}

template <typename Stream>
Awaitable<void> HttpIngress<Stream>::disconnect(sys::error_code const& ec)
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
  co_await http::async_write(stream_, rep);
}

template class HttpIngress<asio::ip::tcp::socket>;
template class HttpIngress<stream::Tls<asio::ip::tcp::socket>>;

template <typename Stream>
HttpEgress<Stream>::HttpEgress(vo::Egress const& vo, IOExecutor const& ex)
requires(std::constructible_from<Stream, IOExecutor const&>)
  : stream_{ex}, peer_{*vo.server_}, credential_{vo}
{
}

template <typename Stream>
HttpEgress<Stream>::HttpEgress(vo::Egress const& vo, IOExecutor const& ex)
requires(stream::TLSStream<Stream>)
  : stream_{stream::tls_context(*vo.tls_, vo.server_->host_), ex},
    peer_{*vo.server_},
    credential_{vo}
{
}

template <typename Stream> Awaitable<size_t> HttpEgress<Stream>::recv(MutableBuffer buf)
{
  if (cache_.size() == 0) co_return co_await stream::read_some(stream_, buf);
  auto copied = std::min(cache_.size(), buf.size());
  std::copy_n(asio::buffers_begin(cache_.data()), copied, std::begin(buf));
  cache_.consume(copied);
  co_return copied;
}

template <typename Stream> Awaitable<void> HttpEgress<Stream>::send(ConstBuffer buf)
{
  co_await stream::write(stream_, buf);
}

template <typename Stream> Awaitable<void> HttpEgress<Stream>::close()
{
  co_await redirect(stream::close(stream_));
}

template <typename Stream> Awaitable<void> HttpEgress<Stream>::connect(Endpoint const& remote)
{
  co_await stream::connect(stream_, peer_);

  auto req = detail::Request{};
  req.method(http::verb::connect);
  if (remote.type_ == EndpointType::IPV6)
    req.target(std::format("[{}:{}]", remote.host_, remote.port_));
  else
    req.target(std::format("{}:{}", remote.host_, remote.port_));
  req.set(http::field::host, req.target());
  req.set(http::field::connection, "close");
  req.set(http::field::proxy_connection, "close");
  credential_.update(req);
  req.prepare_payload();

  co_await http::async_write(stream_, req, asio::use_awaitable);

  auto parser = detail::ResponseParser{};
  co_await http::async_read_header(stream_, cache_, parser, asio::use_awaitable);
  auto code = parser.release().result_int();
  assertTrue(code >= 200 && code < 300, PichiError::CONN_FAILURE);
}

template class HttpEgress<asio::ip::tcp::socket>;
template class HttpEgress<stream::Tls<asio::ip::tcp::socket>>;

}  // namespace pichi::adapter::tcp
