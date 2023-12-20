#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/scope_exit.hpp>
#include <iostream>
#include <pichi/common/asserts.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/common/error.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/crypto/hash.hpp>
#include <pichi/net/helper.hpp>
#include <pichi/net/trojan.hpp>
#include <pichi/stream/test.hpp>
#include <pichi/stream/tls.hpp>
#include <pichi/stream/websocket.hpp>
#include <utility>

using namespace std;
namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace ssl = asio::ssl;
namespace sys = boost::system;
namespace ws = boost::beast::websocket;
using tcp = asio::ip::tcp;

namespace pichi::net {

static constexpr size_t PWD_LEN = crypto::HashTraits<HashAlgorithm::SHA224>::length * 2;

#if __GNUC__ >= 13
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif  // __GNUC__ >= 13

static auto const& HTTP_ERR_CAT = http::make_error_code(http::error::end_of_stream).category();
static auto const& WEBSOCKET_ERR_CAT = ws::make_error_code(ws::error::closed).category();

#if __GNUC__ >= 13
#pragma GCC diagnostic pop
#endif  // __GNUC__ >= 13

static size_t copyToBuffer(ConstBuffer<uint8_t> src, MutableBuffer<uint8_t> dst)
{
  if (src.size() == 0 || dst.size() == 0) return 0;
  auto copied = min(src.size(), dst.size());
  copy_n(cbegin(src), copied, begin(dst));
  return copied;
}

string sha224(string_view pwd)
{
  auto bin = vector(PWD_LEN / 2, 0_u8);
  auto sha224 = crypto::Hash<HashAlgorithm::SHA224>{};
  sha224.hash(ConstBuffer<uint8_t>{pwd}, bin);
  return crypto::bin2hex(bin);
}

template <typename T> struct IsWsStream : public std::false_type {
};
template <typename T> struct IsWsStream<stream::WsStream<T>> : public std::true_type {
};
template <typename T> constexpr bool IsWsStreamV = IsWsStream<T>::value;

template <typename Stream> class StreamWrapper : public Adapter {
public:
  StreamWrapper(Stream& stream) : stream_{stream} {}

  size_t recv(MutableBuffer<uint8_t> buf, Yield yield) override
  {
    return readSome(stream_, buf, yield);
  }

  void send(ConstBuffer<uint8_t> buf, Yield yield) override { write(stream_, buf, yield); }

  void close(Yield yield) override { pichi::net::close(stream_, yield); }

  bool readable() const override { return stream_.is_open(); }

  bool writable() const override { return stream_.is_open(); }

private:
  Stream& stream_;
};

template <typename Stream> auto createStreamWrapper(Stream& stream)
{
  return make_unique<StreamWrapper<Stream>>(stream);
}

template <typename Stream>
size_t TrojanIngress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  if (buf_.size() == 0) return delegate_->recv(buf, yield);
  auto copied = copyToBuffer(buf_.cdata(), buf);
  buf_.consume(copied);
  return copied;
}

template <typename Stream> void TrojanIngress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  delegate_->send(buf, yield);
}

template <typename Stream> void TrojanIngress<Stream>::close(Yield yield)
{
  delegate_->close(yield);
}

template <typename Stream> bool TrojanIngress<Stream>::readable() const
{
  return buf_.size() > 0 || delegate_->readable();
}

template <typename Stream> bool TrojanIngress<Stream>::writable() const
{
  return delegate_->writable();
}

template <typename Stream> void TrojanIngress<Stream>::confirm(Yield) {}

template <typename Stream> Endpoint TrojanIngress<Stream>::readRemote(Yield yield)
{
  try {
    accept(stream_, yield);

    /*
     * To act as a real HTTPS server, like Nginx, the trojan ingress has to read the hashed
     *   password in one time, which is as same as the official trojan.
     *
     * Consuming the password section described in trojan protocol specification:
     *   +-----------------------+---------+-----+
     *   | hex(SHA224(password)) |  CRLF   | CMD |
     *   +-----------------------+---------+-----+
     *   |          56           | X'0D0A' |  1  |
     *   +-----------------------+---------+-----+
     */
    auto b = buf_.prepare(512);
    buf_.commit(readSome(stream_, b, yield));

    auto available = b + buf_.size();
    auto p = static_cast<char const*>(buf_.cdata().data());

    assertTrue(buf_.size() > PWD_LEN + 2, PichiError::BAD_PROTO);
    assertTrue(passwords_.find(string{p, PWD_LEN}) != cend(passwords_),
               PichiError::UNAUTHENTICATED);
    assertTrue(string_view{p + PWD_LEN, 3} == "\r\n\x1"sv, PichiError::BAD_PROTO);

    auto parsed = PWD_LEN + 3;

    /*
     * Parsing the trojan request section:
     *   +------+----------+----------+---------+
     *   | ATYP | DST.ADDR | DST.PORT |  CRLF   |
     *   +------+----------+----------+---------+
     *   |  1   | Variable |    2     | X'0D0A' |
     *   +------+----------+----------+---------+
     * Only CONNECT X'01' CMD is supported, and UDP ASSOCIATE X'03' is unimplemented.
     */
    auto ret = parseEndpoint([&, this, yield, b = b + parsed](auto demand) mutable {
      if (parsed < buf_.size()) {
        auto copied = copyToBuffer({b, buf_.size() - parsed}, demand);
        b += copied;
        demand += copied;
        parsed += copied;
      }
      if (demand.size() > 0) {
        read(stream_, demand, yield);
        copyToBuffer(demand, available);
        buf_.commit(demand.size());
        available += demand.size();
        parsed += demand.size();
      }
    });

    if (buf_.size() - parsed < 2) {
      auto n = parsed + 2 - buf_.size();
      read(stream_, {available, n}, yield);
      buf_.commit(n);
      available += n;
    }
    assertTrue(string_view{p + parsed, 2} == "\r\n"sv, PichiError::BAD_PROTO);
    parsed += 2;

    buf_.consume(parsed);
    delegate_ = createStreamWrapper(stream_);
    return ret;
  }
  catch (sys::system_error const& e) {
#if __GNUC__ >= 13
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif  // __GNUC__ >= 13

    auto&& cat = e.code().category();

#if __GNUC__ >= 13
#pragma GCC diagnostic pop
#endif  // __GNUC__ >= 13

    if (cat != PICHI_CATEGORY && cat != HTTP_ERR_CAT && cat != WEBSOCKET_ERR_CAT) throw e;

    cout << "Trojan Error: " << e.what() << endl;

    if constexpr (IsWsStreamV<Stream>) {
      if (cat == HTTP_ERR_CAT || cat == WEBSOCKET_ERR_CAT) {
        assert(buf_.size() == 0);
        buf_ = stream_.releaseBuffer();
      }
      delegate_ = createStreamWrapper(stream_.next_layer());
    }
    else
      delegate_ = createStreamWrapper(stream_);
    return remote_;
  }
}

template <typename Stream>
size_t TrojanEgress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(stream_, buf, yield);
}

template <typename Stream> void TrojanEgress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  write(stream_, buf, yield);
}

template <typename Stream> void TrojanEgress<Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <typename Stream> bool TrojanEgress<Stream>::readable() const { return stream_.is_open(); }

template <typename Stream> bool TrojanEgress<Stream>::writable() const { return stream_.is_open(); }

template <typename Stream>
void TrojanEgress<Stream>::connect(Endpoint const& remote, ResolveResults next, Yield yield)
{
  pichi::net::connect(next, stream_, yield);

  auto buf = array<uint8_t, 512>{};
  auto written = [&](auto p) -> size_t { return distance(buf.data(), p); };

  /*
   * Here's the trojan protocol specification(https://trojan-gfw.github.io/trojan/protocol):
   *   +-----------------------+---------+-----+------+----------+----------+---------+
   *   | hex(SHA224(password)) |  CRLF   | CMD | ATYP | DST.ADDR | DST.PORT |  CRLF   |
   *   +-----------------------+---------+-----+------+----------+----------+---------+
   *   |          56           | X'0D0A' |  1  |  1   | Variable |    2     | X'0D0A' |
   *   +-----------------------+---------+-----+------+----------+----------+---------+
   */

  copy(cbegin(password_), cend(password_), begin(buf));
  auto p = buf.data() + password_.size();
  *p++ = '\r';
  *p++ = '\n';
  *p++ = 1_u8;
  p += serializeEndpoint(remote, {p, 512 - written(p)});
  *p++ = '\r';
  *p++ = '\n';
  write(stream_, {buf.data(), written(p)}, yield);
}

using TlsStream = stream::TlsStream<tcp::socket>;
using WssStream = stream::WsStream<TlsStream>;
template class TrojanIngress<TlsStream>;
template class TrojanEgress<TlsStream>;
template class TrojanIngress<WssStream>;
template class TrojanEgress<WssStream>;

#ifdef BUILD_TEST
template class TrojanIngress<stream::TestStream>;
template class TrojanEgress<stream::TestStream>;
#endif  // BUILD_TEST

}  // namespace pichi::net
