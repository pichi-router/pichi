#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/write.hpp>
#include <limits>
#include <pichi/asserts.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/http.hpp>
#include <pichi/uri.hpp>

using namespace std;
using namespace std::string_view_literals;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

namespace pichi::net {

struct ParseOnlyBody {
  using value_type = int;

  struct reader {
    template <bool isRequest, typename Fields>
    explicit reader(http::header<isRequest, Fields>&, value_type&)
    {
    }

    template <typename Unused> void init(Unused&&, sys::error_code& ec) { ec = {}; }

    template <typename ConstBufferSeq>
    size_t put(ConstBufferSeq const& buffers, sys::error_code& ec)
    {
      ec = {};
      return asio::buffer_size(buffers);
    }

    void finish(sys::error_code& ec) { ec = {}; }
  };
};

using Socket = asio::ip::tcp::socket;
using Yield = asio::yield_context;

class HttpRelayIngress : public Ingress {
public:
  using Body = http::buffer_body;
  using Buffer = beast::flat_buffer;
  using RequestParser = http::request_parser<Body>;
  using RequestSerializer = http::request_serializer<Body>;
  using ResponseParser = http::response_parser<ParseOnlyBody>;

public:
  HttpRelayIngress(Socket&& socket, Buffer&& buffer,
                   http::request_parser<http::empty_body>&& parser)
    : socket_{move(socket)}, reqBuf_{move(buffer)}, reqParser_{move(parser)}, reqSerializer_{
                                                                                  reqParser_.get()}
  {
    respParser_.header_limit(numeric_limits<uint32_t>::max());
    respParser_.body_limit(numeric_limits<uint64_t>::max());
    reqSerializer_.split(true);
  }

  ~HttpRelayIngress() override = default;

  void close() override { socket_.close(); }

  bool readable() const override
  {
    return socket_.is_open() && !(reqParser_.is_done() && reqSerializer_.is_header_done());
  }

  bool writable() const override { return socket_.is_open() && !respParser_.is_done(); }

  Endpoint readRemote(Yield) override
  {
    fail(PichiError::MISC, "Shouldn't invoke HttpRelayIngress::readRemote");
    return {};
  }

  void confirm(Yield) override {}

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;

private:
  Socket socket_;

  Buffer reqBuf_;
  RequestParser reqParser_;
  mutable RequestSerializer reqSerializer_;

  ResponseParser respParser_;
  Buffer respBuf_;
};

void HttpRelayIngress::send(ConstBuffer<uint8_t> src, Yield yield)
{
  auto b = respBuf_.prepare(src.size());
  copy_n(cbegin(src), src.size(), asio::buffers_begin(b));
  respBuf_.commit(src.size());

  auto ec = sys::error_code{};

  while (respBuf_.size() != 0 && !respParser_.is_done()) {
    auto parsed = respParser_.put(beast::buffers_prefix(respBuf_.size(), respBuf_.data()), ec);
    assertFalse(ec && ec != http::error::need_more, PichiError::BAD_PROTO);
    if (parsed == 0) break;
    asio::async_write(socket_, beast::buffers_prefix(parsed, respBuf_.data()), yield);
    respBuf_.consume(parsed);
  }
}

size_t HttpRelayIngress::recv(MutableBuffer<uint8_t> dst, Yield yield)
{
  assertTrue(reqParser_.is_header_done(), PichiError::MISC);

  auto ec = sys::error_code{};
  if (!reqSerializer_.is_header_done()) {
    auto copied = dst.size();
    reqSerializer_.next(ec, [this, dst, &copied](auto& ec, auto const& buffer) {
      ec = {};
      if (copied > asio::buffer_size(buffer)) copied = asio::buffer_size(buffer);
      copy_n(asio::buffers_begin(buffer), copied, begin(dst));
      reqSerializer_.consume(copied);
    });
    return copied;
  }

  reqParser_.get().body().data = dst.data();
  reqParser_.get().body().size = dst.size();

  auto len = http::async_read_some(socket_, reqBuf_, reqParser_, yield[ec]);
  assertFalse(ec && ec != http::error::need_buffer, PichiError::BAD_PROTO);

  return len;
}

class HttpConnectIngress : public Ingress {
public:
  HttpConnectIngress(Socket&& socket) : socket_{move(socket)} {}
  ~HttpConnectIngress() override = default;

  size_t recv(MutableBuffer<uint8_t> dst, Yield yield) override
  {
    return socket_.async_read_some(asio::buffer(dst), yield);
  }

  void send(ConstBuffer<uint8_t> src, Yield yield) override { write(socket_, src, yield); }

  void close() override { socket_.close(); }
  bool readable() const override { return socket_.is_open(); }
  bool writable() const override { return socket_.is_open(); }

  Endpoint readRemote(Yield) override
  {
    fail(PichiError::MISC, "Shouldn't invoke HttpConnectIngress::readRemote");
    return {};
  }

  void confirm(Yield) override;

private:
  Socket socket_;
};

void HttpConnectIngress::confirm(Yield yield)
{
  auto rep = http::response<http::empty_body>{};
  rep.version(11);
  rep.result(http::status::ok);
  rep.reason("Connection Established");

  http::async_write(socket_, rep, yield);
}

Endpoint HttpIngress::readRemote(Yield yield)
{
  auto buf = HttpRelayIngress::Buffer{};
  auto parser = http::request_parser<http::empty_body>{};

  parser.header_limit(numeric_limits<uint32_t>::max());
  parser.body_limit(numeric_limits<uint64_t>::max());

  http::async_read_header(socket_, buf, parser, yield);

  auto& header = parser.get();
  if (header.method() == http::verb::connect) {
    /*
     * HTTP CONNECT @RFC2616
     *   Don't validate whether the HOST field exists or not here.
     *   Some clients are not standard and send the CONNECT request without HOST field.
     */
    auto hp = HostAndPort{{header.target().data(), header.target().size()}};
    delegate_ = make_unique<HttpConnectIngress>(move(socket_));
    return {detectHostType(hp.host_), to_string(hp.host_), to_string(hp.port_)};
  }
  else {
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
    auto target = header.target().to_string();
    assertFalse(target.empty(), PichiError::BAD_PROTO, "Empty path");
    if (target[0] != '/') {
      // absolute_path specified, so convert it to relative one.
      auto uri = Uri{target};
      header.target(boost::string_view{uri.suffix_.data(), uri.suffix_.size()});
    }

    auto it = header.find(http::field::host);
    assertTrue(it != cend(header), PichiError::BAD_PROTO, "Missing HOST field in HTTP header");
    auto hp = HostAndPort{{it->value().data(), it->value().size()}};

    delegate_ = make_unique<HttpRelayIngress>(move(socket_), move(buf), move(parser));
    return {detectHostType(hp.host_), to_string(hp.host_), to_string(hp.port_)};
  }
}

class HttpConnectEgress : public Egress {
public:
  HttpConnectEgress(Socket& s) : socket_{s} {}

  void close() override { socket_.close(); }
  bool readable() const override { return socket_.is_open(); }
  bool writable() const override { return socket_.is_open(); }
  size_t recv(MutableBuffer<uint8_t> buf, Yield yield) override
  {
    return socket_.async_read_some(asio::buffer(buf), yield);
  }
  void send(ConstBuffer<uint8_t> buf, Yield yield) override { write(socket_, buf, yield); }
  void connect(Endpoint const&, Endpoint const&, Yield) override;

private:
  Socket& socket_;
};

void HttpConnectEgress::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
{
  pichi::net::connect(next, socket_, yield);

  auto req = http::request<http::empty_body>{};
  req.method(http::verb::connect);
  req.target(remote.host_ + ":" + remote.port_);
  req.prepare_payload();

  http::async_write(socket_, req, yield);

  auto resp = http::response<http::empty_body>{};
  auto parser = http::response_parser<http::empty_body>{resp};
  auto buf = beast::flat_buffer{};
  http::async_read_header(socket_, buf, parser, yield);
  assertTrue(resp.result() == http::status::ok, PichiError::BAD_PROTO,
             "Failed to establish connection with " + remote.host_ + ":" + remote.port_);
}

class HttpRelayEgress : public Egress {
private:
  using Buffer = beast::flat_buffer;
  template <bool isRequest> using Header = http::header<isRequest>;
  template <bool isRequest> using Parser = http::parser<isRequest, ParseOnlyBody>;
  template <bool isRequest> using Serializer = http::serializer<isRequest, ParseOnlyBody>;
  using RequestParser = Parser<true>;
  using ResponseParser = Parser<false>;

  void rewriteTarget(Header<true>& h)
  {
    auto it = h.find(http::field::host);
    assertFalse(it == cend(h), PichiError::BAD_PROTO);
    auto prefix = "http://"s;
    prefix.append(cbegin(it->value()), cend(it->value()));
    prefix.append(cbegin(h.target()), cend(h.target()));
    h.target(prefix);
  }

  template <bool isRequest> void writeHeader(Header<isRequest>& h, asio::yield_context yield)
  {
    auto msg = http::message<isRequest, http::empty_body>{h};
    auto sr = http::serializer<isRequest, http::empty_body>{msg};
    http::async_write_header(socket_, sr, yield);
  }

public:
  HttpRelayEgress(Socket& s) : socket_{s} { respParser_.eager(true); }

  void close() override { socket_.close(); }
  bool readable() const override
  {
    return socket_.is_open() && (!respParser_.is_done() || parsed_ > 0);
  }
  bool writable() const override { return socket_.is_open() && !reqParser_.is_done(); }

  size_t recv(MutableBuffer<uint8_t> buf, Yield yield) override
  {
    if (parsed_ == 0) {
      assertFalse(respParser_.is_done(), PichiError::BAD_PROTO);
      respBuf_.commit(socket_.async_read_some(respBuf_.prepare(1024), yield));
      auto ec = sys::error_code{};
      parsed_ += respParser_.put(beast::buffers_prefix(respBuf_.size(), respBuf_.data()), ec);
      assertFalse(ec && ec != http::error::need_more, PichiError::BAD_PROTO, ec.message());
    }
    auto copied = min(buf.size(), parsed_);
    copy_n(asio::buffers_begin(beast::buffers_prefix(copied, respBuf_.data())), copied, begin(buf));
    parsed_ -= copied;
    respBuf_.consume(copied);
    return copied;
  }

  void send(ConstBuffer<uint8_t> buf, Yield yield) override
  {
    if (reqParser_.is_header_done()) {
      auto ec = sys::error_code{};
      auto parsed = reqParser_.put(asio::buffer(buf), ec);
      assertFalse(ec && ec != http::error::need_more, PichiError::BAD_PROTO, ec.message());
      assertTrue(parsed == buf.size());
      write(socket_, buf, yield);
    }
    else {
      copy_n(cbegin(buf), buf.size(), asio::buffers_begin(reqBuf_.prepare(buf.size())));
      reqBuf_.commit(buf.size());
      auto ec = sys::error_code{};
      auto parsed = reqParser_.put(beast::buffers_prefix(reqBuf_.size(), reqBuf_.data()), ec);
      reqBuf_.consume(parsed);
      if (ec == http::error::need_more) return;
      assertFalse(static_cast<bool>(ec), PichiError::BAD_PROTO, ec.message());
      assertTrue(reqParser_.is_header_done());

      rewriteTarget(reqParser_.get());
      writeHeader(reqParser_.get(), yield);
      if (reqBuf_.size() > 0) send({reqBuf_.data()}, yield);
      reqBuf_.consume(reqBuf_.size());
    }
  }

  void connect(Endpoint const& remote, Endpoint const& next, Yield yield) override
  {
    pichi::net::connect(next, socket_, yield);
  }

private:
  Socket& socket_;
  RequestParser reqParser_;
  ResponseParser respParser_;
  Buffer reqBuf_;
  Buffer respBuf_;
  size_t parsed_ = 0;
};

HttpEgress::HttpEgress(Socket&& socket) : socket_{move(socket)} {}

void HttpEgress::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
{
  // FIXME hardcode for detecting which proxy type will be chosen.
  if (remote.port_ == "https" || remote.port_ == "443")
    delegate_ = make_unique<HttpConnectEgress>(socket_);
  else
    delegate_ = make_unique<HttpRelayEgress>(socket_);
  delegate_->connect(remote, next, yield);
}

} // namespace pichi::net
