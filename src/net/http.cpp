#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/write.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/common/uri.hpp>
#include <pichi/crypto/base64.hpp>
#include <pichi/net/helper.hpp>
#include <pichi/net/http.hpp>
#include <pichi/stream/test.hpp>
#include <pichi/stream/tls.hpp>
#include <regex>
#include <sstream>

using namespace std;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
using tcp = asio::ip::tcp;

namespace pichi::net {

namespace detail {

using OptCredential = optional<pair<string, string>>;

static auto const BASIC_AUTH_PATTERN = regex{"^basic ([a-z0-9+/]+={0,2})", regex::icase};

template <bool isRequest> using Serializer = boost::beast::http::serializer<isRequest, Body>;

static void assertNoError(sys::error_code ec)
{
  if (ec) throw sys::system_error{ec};
}

template <typename ConstBufferSequence>
static size_t copyOrCache(ConstBufferSequence const& src, MutableBuffer<uint8_t> dst, Cache& cache)
{
  auto first = asio::buffers_begin(src);
  auto n = asio::buffer_size(src);
  auto copied = min(n, dst.size());
  copy_n(first, copied, begin(dst));
  if (n > copied) {
    copy_n(first + copied, n - copied, asio::buffers_begin(cache.prepare(n - copied)));
    cache.commit(n - copied);
  }
  return copied;
}

template <typename Stream, typename DynamicBuffer, typename Yield>
static size_t recvRaw(Stream& s, DynamicBuffer& cache, MutableBuffer<uint8_t> buf, Yield yield)
{
  if (cache.size() == 0) return readSome(s, buf, yield);
  auto copied = min(buf.size(), cache.size());
  copy_n(asio::buffers_begin(cache.data()), copied, begin(buf));
  cache.consume(copied);
  return copied;
}

template <bool isRequest, typename DynamicBuffer>
static size_t recvHeader(Header<isRequest> const& header, DynamicBuffer& cache,
                         MutableBuffer<uint8_t> buf)
{
  auto m = Message<isRequest>{header};
  auto sr = Serializer<isRequest>{m};
  auto left = buf;
  auto stickyLen = cache.size();
  do {
    auto ec = sys::error_code{};
    sr.next(ec, [&](auto&&, auto&& serialized) {
      left += copyOrCache(serialized, left, cache);
      sr.consume(asio::buffer_size(serialized));
    });
    assertNoError(ec);
  } while (!sr.is_header_done());
  asio::buffer_copy(cache.prepare(stickyLen), cache.data());
  cache.commit(stickyLen);
  cache.consume(stickyLen);
  return buf.size() - left.size();
}

static void addHostToTarget(http::request_header<>& req)
{
  auto it = req.find(http::field::host);
  assertTrue(it != cend(req), PichiError::BAD_PROTO, "Missing HOST field in HTTP header");
  auto target = "http://"s;
  target.append(cbegin(it->value()), cend(it->value()));
  target.append(cbegin(req.target()), cend(req.target()));
  req.target(target);
}

template <bool isRequest> static void addCloseHeader(Header<isRequest>& header)
{
  /*
   * FIXME Pichi doesn't actually do active closing. We wish upstream server
   *   could work correctly if we set 'close' header.
   */
  header.set(http::field::connection, "close");
  header.set(http::field::proxy_connection, "close");
}

static void addAuthenticationHeader(Header<true>& req, OptCredential const& cred)
{
  if (cred.has_value())
    req.set(http::field::proxy_authorization,
            "Basic "s + crypto::base64Encode(""s + cred->first + ":" + cred->second));
}

template <typename Stream, typename Yield> static void tunnelConfirm(Stream& s, Yield yield)
{
  auto rep = Response{};
  rep.version(11);
  rep.result(http::status::ok);
  rep.reason("Connection Established");
  addCloseHeader(rep);
  writeHttp(s, rep, yield);
}

template <typename Stream, typename Yield>
static bool tunnelConnect(Endpoint const& remote, Stream& s, Yield yield,
                          OptCredential const& cred = {})
{
  auto oss = ostringstream{};
  oss << (remote.type_ == EndpointType::IPV6 ? "[" + remote.host_ + "]" : remote.host_) << ":"
      << remote.port_;
  auto host = oss.str();
  auto req = Request{};
  req.method(http::verb::connect);
  req.target(host);
  req.set(http::field::host, host);
  addCloseHeader(req);
  addAuthenticationHeader(req, cred);
  req.prepare_payload();

  writeHttp(s, req, yield);

  auto parser = ResponseParser{};
  auto cache = Cache{};
  readHttpHeader(s, cache, parser, yield);
  auto code = parser.release().result_int();

  return code >= 200 && code < 300;
}

template <typename DynamicBuffer>
static auto copyToCache(ConstBuffer<uint8_t> buf, DynamicBuffer& cache)
{
  copy(cbegin(buf), cend(buf), asio::buffers_begin(cache.prepare(buf.size())));
  cache.commit(buf.size());
  return cache.data();
}

template <bool isRequest, typename DynamicBuffer>
static ConstBuffer<uint8_t> parseHeader(Parser<isRequest>& parser, DynamicBuffer& cache,
                                        ConstBuffer<uint8_t> buf)
{
  assertFalse(parser.is_header_done());

  // Parse from cache if it has to happen.
  auto fromCache = cache.size() > 0;
  auto ec = sys::error_code{};
  auto parsed = parser.put(fromCache ? copyToCache(buf, cache) : asio::buffer(buf), ec);
  if (ec == http::error::need_more) ec = {};
  assertNoError(ec);

  if (fromCache) {
    cache.consume(parsed);
    return cache.data();
  }
  else {
    // Unparsed data left in buf has to be stored in cache for next sending.
    auto left = buf + parsed;
    copyToCache(left, cache);
    return left;
  }
}

template <bool isRequest, typename DynamicBuffer, typename Stream, typename Yield>
static bool tryToSendHeader(Parser<isRequest>& parser, DynamicBuffer& cache,
                            ConstBuffer<uint8_t> buf, Stream& s, Yield yield,
                            OptCredential const& cred = {})
{
  suppressC4100(cred);

  auto left = parseHeader(parser, cache, buf);
  if (!parser.is_header_done()) return false;

  auto m = parser.release();
  if (!parser.upgrade()) addCloseHeader(m);
  if constexpr (isRequest) {
    addHostToTarget(m);
    addAuthenticationHeader(m, cred);
  }
  writeHttpHeader(s, Serializer<isRequest>{m}, yield);

  write(s, left, yield);
  cache.consume(cache.size());

  return true;
}

template <typename Request> auto getUsernameAndPassword(Request const& req)
{
  auto auth = req.find(http::field::proxy_authorization);
  assertFalse(auth == cend(req), PichiError::BAD_AUTH_METHOD);
  auto m = cmatch{};
  auto credential = auth->value();
  assertTrue(regex_match(cbegin(credential), cend(credential), m, BASIC_AUTH_PATTERN),
             PichiError::BAD_AUTH_METHOD);
  assertTrue(2 == m.size() && m[1].matched, PichiError::BAD_AUTH_METHOD);
  auto plain = crypto::base64Decode({m[1].first, static_cast<size_t>(m[1].length())},
                                    PichiError::BAD_AUTH_METHOD);
  auto colon = plain.find_first_of(':');
  assertFalse(colon == string::npos, PichiError::BAD_AUTH_METHOD);
  return make_pair(plain.substr(0, colon), plain.substr(colon + 1));
}

}  // namespace detail

using namespace detail;

template <typename Stream> size_t HttpIngress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return recv_(buf, yield);
}

template <typename Stream> void HttpIngress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  send_(buf, yield);
}

template <typename Stream> bool HttpIngress<Stream>::readable() const
{
  return stream_.is_open() || reqCache_.size() > 0;
}

template <typename Stream> bool HttpIngress<Stream>::writable() const { return stream_.is_open(); }

template <typename Stream> void HttpIngress<Stream>::confirm(Yield yield) { confirm_(yield); }

template <typename Stream> void HttpIngress<Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <typename Stream> void HttpIngress<Stream>::disconnect(exception_ptr eptr, Yield)
{
  auto rep = make_unique<Response>();
  rep->version(11);
  rep->set(http::field::connection, "Close");
  try {
    rethrow_exception(eptr);
  }
  catch (Exception const& e) {
    switch (e.error()) {
    case PichiError::CONN_FAILURE:
      rep->result(http::status::gateway_timeout);
      break;
    case PichiError::BAD_AUTH_METHOD:
      rep->result(http::status::proxy_authentication_required);
      rep->set(http::field::proxy_authenticate, "Basic");
      break;
    case PichiError::UNAUTHENTICATED:
      rep->result(http::status::forbidden);
      break;
    default:
      rep->result(http::status::internal_server_error);
      break;
    }
  }
  catch (sys::system_error const& e) {
    // TODO classify these errors

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
    auto pCat = dynamic_cast<CategoryPtr>(&e.code().category());
    rep->result(pCat ? http::status::bad_request : http::status::gateway_timeout);
  }
  /*  FIXME Windows+MSVC2017/2019
   *  It is supposed to be investigated in the future that the 'rep' on the stack
   *  has already been destructed before the WriteHandler, 'yield' here, is invoked.
   *  This case occurs randomly when concurrently writing.
   */
  auto& r = *rep;
  writeHttp(stream_, r, [rep = move(rep)](auto&&, auto) {
    // ignore all errors here
  });
}

template <typename Stream> Endpoint HttpIngress<Stream>::readRemote(Yield yield)
{
  accept(stream_, yield);

  readHttpHeader(stream_, reqCache_, reqParser_, yield);
  auto& req = reqParser_.get();
  if (auth_.has_value()) {
    auto [username, password] = getUsernameAndPassword(req);
    assertTrue(invoke(*auth_, username, password), PichiError::UNAUTHENTICATED);
    req.erase(http::field::proxy_authorization);
  }

  if (req.method() == http::verb::connect) {
    send_ = [this](auto buf, auto yield) { write(stream_, buf, yield); };
    recv_ = [this](auto buf, auto yield) {
      if (reqCache_.size() == 0) return readSome(stream_, buf, yield);

      auto copied = min(buf.size(), reqCache_.size());
      copy_n(asio::buffers_begin(reqCache_.data()), copied, begin(buf));
      reqCache_.consume(copied);
      return copied;
    };
    confirm_ = [this](auto yield) {
      tunnelConfirm(stream_, yield);
      reqParser_.release();
    };
    /*
     * HTTP CONNECT @RFC2616
     *   Don't validate whether the HOST field exists or not here.
     *   Some clients are not standard and send the CONNECT request without HOST
     * field.
     */
    auto hp = HostAndPort{{req.target().data(), req.target().size()}};
    return makeEndpoint(hp.host_, hp.port_);
  }
  else {
    send_ = [this](auto buf, auto yield) {
      if (tryToSendHeader(respParser_, respCache_, buf, stream_, yield))
        send_ = [this](auto buf, auto yield) { write(stream_, buf, yield); };
    };
    recv_ = [this](auto buf, auto) {
      recv_ = [this](auto buf, auto yield) { return recvRaw(stream_, reqCache_, buf, yield); };
      auto req = reqParser_.release();
      if (!reqParser_.upgrade()) addCloseHeader(req);
      return recvHeader(req, reqCache_, buf);
    };
    confirm_ = [](auto) {};

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
    auto target = req.target().to_string();
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
      assertTrue(it != cend(req), PichiError::BAD_PROTO, "Lack of target information");
      auto hp = HostAndPort{{it->value().data(), it->value().size()}};
      return makeEndpoint(hp.host_, hp.port_);
    }
  }
}

template <typename Stream>
void HttpEgress<Stream>::connect(Endpoint const& remote, ResolveResults next, Yield yield)
{
  pichi::net::connect(next, stream_, yield);
  if (tunnelConnect(remote, stream_, yield, credential_)) {
    send_ = [this](auto buf, auto yield) { write(stream_, buf, yield); };
    recv_ = [this](auto buf, auto yield) { return readSome(stream_, buf, yield); };
    return;
  }

  // Failed to make connection via HTTP CONNECT, so try HTTP relay again.
  send_ = [this](auto buf, auto yield) {
    if (tryToSendHeader(reqParser_, reqCache_, buf, stream_, yield, credential_))
      send_ = [this](auto buf, auto yield) { write(stream_, buf, yield); };
  };
  recv_ = [this](auto buf, auto yield) { return recvRaw(stream_, respCache_, buf, yield); };

  pichi::net::close(stream_, yield);
  pichi::net::connect(next, stream_, yield);
}

template <typename Stream> size_t HttpEgress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return recv_(buf, yield);
}

template <typename Stream> void HttpEgress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  send_(buf, yield);
}

template <typename Stream> void HttpEgress<Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <typename Stream> bool HttpEgress<Stream>::readable() const
{
  return stream_.is_open() || respCache_.size() > 0;
}

template <typename Stream> bool HttpEgress<Stream>::writable() const { return stream_.is_open(); }

using TcpSocket = tcp::socket;

template class HttpIngress<TcpSocket>;
template class HttpEgress<TcpSocket>;

using TLSStream = stream::TlsStream<TcpSocket>;
template class HttpIngress<TLSStream>;
template class HttpEgress<TLSStream>;

#ifdef BUILD_TEST
template class HttpIngress<stream::TestStream>;
template class HttpEgress<stream::TestStream>;
#endif  // BUILD_TEST

}  // namespace pichi::net
