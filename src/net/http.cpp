#include "config.h"
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/write.hpp>
#include <limits>
#include <pichi/asserts.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/http.hpp>
#include <pichi/uri.hpp>

#ifdef ENABLE_TLS
#include <boost/asio/ssl/stream.hpp>
#endif // ENABLE_TLS

using namespace std;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = asio::ssl;
namespace sys = boost::system;
using tcp = asio::ip::tcp;

namespace pichi::net {

namespace detail {

template <bool isRequest> using Header = boost::beast::http::header<isRequest>;
template <bool isRequest> using Message = boost::beast::http::message<isRequest, Body>;
using Request = Message<true>;
using Response = Message<false>;
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

template <bool isRequest, typename DynamicBuffer>
static size_t parseFromBuffer(Parser<isRequest>& parser, DynamicBuffer& cache,
                              ConstBuffer<uint8_t> data)
{
  assertFalse(parser.is_header_done());
  auto fromCache = cache.size() > 0;
  if (fromCache) {
    copy(cbegin(data), cend(data), asio::buffers_begin(cache.prepare(data.size())));
    cache.commit(data.size());
  }
  auto ec = sys::error_code{};
  auto parsed = parser.put(fromCache ? cache.data() : asio::buffer(data), ec);
  if (ec == http::error::need_more) ec = {};
  assertNoError(ec);
  if (fromCache) {
    parsed -= cache.size();
    cache.consume(parsed);
  }
  else {
    copy(cbegin(data) + parsed, cend(data),
         asio::buffers_begin(cache.prepare(data.size() - parsed)));
  }
  return parsed;
}

template <typename Stream, typename DynamicBuffer>
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
  do {
    auto ec = sys::error_code{};
    sr.next(ec, [&](auto&&, auto&& serialized) {
      left += copyOrCache(serialized, left, cache);
      sr.consume(asio::buffer_size(serialized));
    });
    assertNoError(ec);
  } while (!sr.is_header_done());
  return buf.size() - left.size();
}

template <typename Stream, bool isRequest, typename DynamicBuffer>
static void sendHeader(Stream& s, Header<isRequest>& header, DynamicBuffer& cache, Yield yield)
{
  auto m = Message<isRequest>{header};
  http::async_write(s, m, yield);
  asio::async_write(s, cache.data(), yield);
  cache.consume(cache.size());
}

static void removeHostFromTarget(http::request_header<>& req)
{
  /*
   * HTTP Proxy @RFC2068
   *   The HOST field and absolute_path are both mandatory and same according to the standard.
   *   But some non-standard clients might send request:
   *     - with different destinations discribed in HOST field and absolute_path;
   *     - without absolute_path but relative_path specified.
   *   The rules, which are not very strict but still standard, listed below are followed
   *   to handle these non-standard clients:
   *     - HOST field is mandatory and taken as the destination;
   *     - the destination described in absolute_path is ignored;
   *     - relative_path will be forwarded without any change.
   */
  auto target = req.target().to_string();
  assertFalse(target.empty(), PichiError::BAD_PROTO, "Empty path");
  if (target[0] != '/') {
    // absolute_path specified, so convert it to relative one.
    auto uri = Uri{target};
    req.target(boost::string_view{uri.suffix_.data(), uri.suffix_.size()});
  }
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
  header.set(http::field::connection, "close"sv);
  header.set(http::field::proxy_connection, "close"sv);
}

template <typename Stream> static void tunnelConfirm(Stream& s, Yield yield)
{
  auto rep = Response{};
  rep.version(11);
  rep.result(http::status::ok);
  rep.reason("Connection Established");
  addCloseHeader(rep);
  http::async_write(s, rep, yield);
}

template <typename Stream> static void tunnelConnect(Endpoint const& remote, Stream& s, Yield yield)
{
  auto host = remote.host_ + ":" + remote.port_;
  auto req = Request{};
  req.method(http::verb::connect);
  req.target(host);
  req.set(http::field::host, host);
  addCloseHeader(req);
  req.prepare_payload();

  http::async_write(s, req, yield);

  auto parser = ResponseParser{};
  auto cache = Cache{};
  http::async_read_header(s, cache, parser, yield);
  auto resp = parser.release();

  assertTrue(resp.result_int() / 100 == 2, PichiError::BAD_PROTO,
             "Failed to establish connection with " + remote.host_ + ":" + remote.port_);
}

} // namespace detail

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
  return isOpen(stream_) || reqCache_.size() > 0;
}

template <typename Stream> bool HttpIngress<Stream>::writable() const { return isOpen(stream_); }

template <typename Stream> void HttpIngress<Stream>::confirm(Yield yield) { confirm_(yield); }

template <typename Stream> void HttpIngress<Stream>::close() { pichi::net::close(stream_); }

template <typename Stream> void HttpIngress<Stream>::disconnect(Yield yield)
{
  auto ec = sys::error_code{};
  auto rep = http::response<http::empty_body>{};
  rep.version(11);
  rep.result(http::status::request_timeout);
  // Ignoring all errors here
  http::async_write(stream_, rep, yield[ec]);
}

template <typename Stream> Endpoint HttpIngress<Stream>::readRemote(Yield yield)
{
#ifdef ENABLE_TLS
  if constexpr (IsSslStreamV<Stream>) {
    stream_.async_handshake(ssl::stream_base::handshake_type::server, yield);
  }
#endif // ENABLE_TLS

  http::async_read_header(stream_, reqCache_, reqParser_, yield);

  auto& req = reqParser_.get();
  if (req.method() == http::verb::connect) {
    send_ = [this](auto buf, auto yield) { write(stream_, buf, yield); };
    recv_ = [this](auto buf, auto yield) { return readSome(stream_, buf, yield); };
    confirm_ = [this](auto yield) {
      tunnelConfirm(stream_, yield);
      reqParser_.release();
    };

    /*
     * HTTP CONNECT @RFC2616
     *   Don't validate whether the HOST field exists or not here.
     *   Some clients are not standard and send the CONNECT request without HOST field.
     */
    auto hp = HostAndPort{{req.target().data(), req.target().size()}};
    return {detectHostType(hp.host_), to_string(hp.host_), to_string(hp.port_)};
  }
  else {
    send_ = [this](auto buf, auto yield) {
      auto consumed = parseFromBuffer(respParser_, respCache_, buf);
      if (!respParser_.is_header_done()) return;
      auto resp = respParser_.release();
      addCloseHeader(resp);
      sendHeader(stream_, resp, respCache_, yield);
      write(stream_, buf + consumed, yield);
      send_ = [this](auto buf, auto yield) { write(stream_, buf, yield); };
    };
    recv_ = [this](auto buf, auto yield) {
      recv_ = [this](auto buf, auto yield) { return recvRaw(stream_, reqCache_, buf, yield); };
      auto req = reqParser_.release();
      addCloseHeader(req);
      return recvHeader(req, reqCache_, buf);
    };
    confirm_ = [](auto) {};

    removeHostFromTarget(req);
    auto it = req.find(http::field::host);
    assertTrue(it != cend(req), PichiError::BAD_PROTO, "Missing HOST field in HTTP header");
    auto hp = HostAndPort{{it->value().data(), it->value().size()}};
    return {detectHostType(hp.host_), to_string(hp.host_), to_string(hp.port_)};
  }
}

template <typename Stream>
void HttpEgress<Stream>::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
{
  pichi::net::connect(next, stream_, yield);
  // FIXME hardcode for detecting which proxy type will be chosen.
  if (remote.port_ == "https" || remote.port_ == "443") {
    send_ = [this](auto buf, auto yield) { write(stream_, buf, yield); };
    recv_ = [this](auto buf, auto yield) { return readSome(stream_, buf, yield); };
    tunnelConnect(remote, stream_, yield);
  }
  else {
    send_ = [this](auto buf, auto yield) {
      auto consumed = parseFromBuffer(reqParser_, reqCache_, buf);
      if (!reqParser_.is_header_done()) return;
      auto req = reqParser_.release();
      addCloseHeader(req);
      addHostToTarget(req);
      sendHeader(stream_, req, reqCache_, yield);
      write(stream_, buf + consumed, yield);
      send_ = [this](auto buf, auto yield) { write(stream_, buf, yield); };
    };
    recv_ = [this](auto buf, auto yield) { return recvRaw(stream_, respCache_, buf, yield); };
  }
}

template <typename Stream> size_t HttpEgress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return recv_(buf, yield);
}

template <typename Stream> void HttpEgress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  send_(buf, yield);
}

template <typename Stream> void HttpEgress<Stream>::close() { pichi::net::close(stream_); }

template <typename Stream> bool HttpEgress<Stream>::readable() const
{
  return isOpen(stream_) || respCache_.size() > 0;
}

template <typename Stream> bool HttpEgress<Stream>::writable() const { return isOpen(stream_); }

using TcpSocket = tcp::socket;
using TlsSocket = ssl::stream<TcpSocket>;

template class HttpIngress<TcpSocket>;
template class HttpEgress<TcpSocket>;

#ifdef ENABLE_TLS
template class HttpIngress<TlsSocket>;
template class HttpEgress<TlsSocket>;
#endif // ENABLE_TLS

} // namespace pichi::net
