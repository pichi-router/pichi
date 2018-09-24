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

class HttpRelay : public Ingress {
public:
  using Body = http::buffer_body;
  using Buffer = beast::flat_buffer;
  using RequestParser = http::request_parser<Body>;
  using RequestSerializer = http::request_serializer<Body>;
  using ResponseParser = http::response_parser<ParseOnlyBody>;

public:
  HttpRelay(Socket&& socket, Buffer&& buffer, http::request_parser<http::empty_body>&& parser)
    : socket_{move(socket)}, reqBuf_{move(buffer)}, reqParser_{move(parser)}, reqSerializer_{
                                                                                  reqParser_.get()}
  {
    respParser_.header_limit(MAX_FRAME_SIZE);
    respParser_.body_limit(numeric_limits<uint64_t>::max());
    reqSerializer_.split(true);
  }

  ~HttpRelay() override = default;

  void close() override { socket_.close(); }

  bool readable() const override
  {
    return socket_.is_open() && !(reqParser_.is_done() && reqSerializer_.is_header_done());
  }

  bool writable() const override { return socket_.is_open() && !respParser_.is_done(); }

  Endpoint readRemote(Yield) override
  {
    fail(PichiError::MISC, "Shouldn't invoke HttpRelay::readRemote");
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

void HttpRelay::send(ConstBuffer<uint8_t> src, Yield yield)
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

size_t HttpRelay::recv(MutableBuffer<uint8_t> dst, Yield yield)
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
  auto buf = HttpRelay::Buffer{};
  auto parser = http::request_parser<http::empty_body>{};

  parser.header_limit(MAX_FRAME_SIZE);
  parser.body_limit(numeric_limits<uint64_t>::max());

  http::async_read_header(socket_, buf, parser, yield);

  auto& header = parser.get();
  if (header.method() == http::verb::connect) {
    assertTrue(header.find(http::field::host) != header.end(), PichiError::BAD_PROTO);
    auto hp = HostAndPort{{header.target().data(), header.target().size()}};
    delegate_ = make_unique<HttpConnectIngress>(move(socket_));
    return {detectHostType(hp.host_), to_string(hp.host_), to_string(hp.port_)};
  }
  else {
    // Cutting http://example.com/suffix?query to /suffix?query
    auto target = header.target().to_string();
    auto uri = Uri{target};
    header.target(boost::string_view{uri.suffix_.data(), uri.suffix_.size()});
    delegate_ = make_unique<HttpRelay>(move(socket_), move(buf), move(parser));
    return {detectHostType(uri.host_), to_string(uri.host_), to_string(uri.port_)};
  }
}

HttpEgress::HttpEgress(Socket&& socket) : socket_{move(socket)} {}

void HttpEgress::close() { return socket_.close(); }
bool HttpEgress::readable() const { return socket_.is_open(); }
bool HttpEgress::writable() const { return socket_.is_open(); }

size_t HttpEgress::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return socket_.async_read_some(asio::buffer(buf), yield);
}

void HttpEgress::send(ConstBuffer<uint8_t> buf, Yield yield) { write(socket_, buf, yield); }

void HttpEgress::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
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

} // namespace pichi::net
