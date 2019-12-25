#include <pichi/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/asserts.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/socks5.hpp>
#include <pichi/test/socket.hpp>
#include <utility>

#ifdef ENABLE_TLS
#include <boost/asio/ssl/stream.hpp>
#endif // ENABLE_TLS

using namespace std;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace sys = boost::system;
using tcp = asio::ip::tcp;

namespace pichi::net {

template <typename T> using HeaderBuffer = array<T, 512>;

template <typename Stream> void Socks5Ingress<Stream>::authenticate(Yield yield)
{
  auto buf = HeaderBuffer<uint8_t>{};

  /*
   * Request:
   * +----+------+----------+------+----------+
   * |VER | ULEN |  UNAME   | PLEN |  PASSWD  |
   * +----+------+----------+------+----------+
   * | 1  |  1   | 1 to 255 |  1   | 1 to 255 |
   * +----+------+----------+------+----------+
   */
  read(stream_, {buf, 2}, yield);
  assertTrue(buf.front() == 0x01_u8, PichiError::BAD_PROTO);

  auto len = static_cast<size_t>(buf[1]);
  assertFalse(len == 0, PichiError::BAD_PROTO);
  read(stream_, {buf, len + 1}, yield);
  auto name = string{cbegin(buf), cbegin(buf) + len};

  len = static_cast<size_t>(buf[len]);
  assertFalse(len == 0, PichiError::BAD_PROTO);
  read(stream_, {buf, len}, yield);
  auto pass = string{cbegin(buf), cbegin(buf) + len};

  auto it = credentials_.find(name);
  assertFalse(it == cend(credentials_), PichiError::UNAUTHENTICATED);
  assertTrue(it->second == pass, PichiError::UNAUTHENTICATED);

  /*
   * Response:
   * +----+--------+
   * |VER | STATUS |
   * +----+--------+
   * | 1  |   1    |
   * +----+--------+
   */
  buf[0] = 0x01_u8;
  buf[1] = 0x00_u8;
  write(stream_, {buf, 2}, yield);
}

template <typename Stream>
size_t Socks5Ingress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(stream_, buf, yield);
}

template <typename Stream> void Socks5Ingress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  write(stream_, buf, yield);
}

template <typename Stream> void Socks5Ingress<Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <typename Stream> bool Socks5Ingress<Stream>::readable() const { return isOpen(stream_); }

template <typename Stream> bool Socks5Ingress<Stream>::writable() const { return isOpen(stream_); }

template <typename Stream> Endpoint Socks5Ingress<Stream>::readRemote(Yield yield)
{
#ifdef ENABLE_TLS
  if constexpr (IsSslStreamV<Stream>) {
    stream_.async_handshake(ssl::stream_base::handshake_type::server, yield);
  }
#endif // ENABLE_TLS

  auto buf = HeaderBuffer<uint8_t>{};

  read(stream_, {buf, 2}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] > 0, PichiError::BAD_PROTO);

  uint8_t len = buf[1];
  read(stream_, {buf, len}, yield);

  auto needAuth = !credentials_.empty();
  auto m = needAuth ? 0x02_u8 : 0x00_u8;
  assertFalse(find(cbegin(buf), cbegin(buf) + len, m) == cbegin(buf) + len,
              PichiError::BAD_AUTH_METHOD);
  buf[0] = 0x05_u8;
  buf[1] = m;
  write(stream_, {buf, 2}, yield);

  if (needAuth) authenticate(yield);

  read(stream_, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x01, PichiError::BAD_PROTO);
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);

  return parseEndpoint([this, yield](auto dst) { read(stream_, dst, yield); });
}

template <typename Stream> void Socks5Ingress<Stream>::confirm(Yield yield)
{
  static uint8_t const buf[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  write(stream_, buf, yield);
}

template <typename Stream> void Socks5Ingress<Stream>::disconnect(PichiError e, Yield yield)
{
  static auto const HANDSHAKE_FAILURE = array{0x05_u8, 0x04_u8, 0x00_u8, 0x01_u8, 0x00_u8,
                                              0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8};
  static auto const AUTH_FAILURE = array{0x01_u8, 0xff_u8};
  static auto const METHOD_FAILURE = array{0x05_u8, 0xff_u8};

  auto ec = sys::error_code{};
  switch (e) {
  case PichiError::CONN_FAILURE:
    write(stream_, HANDSHAKE_FAILURE, yield[ec]);
    break;
  case PichiError::BAD_AUTH_METHOD:
    write(stream_, METHOD_FAILURE, yield[ec]);
    break;
  case PichiError::UNAUTHENTICATED:
    write(stream_, AUTH_FAILURE, yield[ec]);
    break;
  default:
    break;
  }
}

template class Socks5Ingress<tcp::socket>;

#ifdef ENABLE_TLS
template class Socks5Ingress<ssl::stream<tcp::socket>>;
#endif // ENABLE_TLS

#ifdef BUILD_TEST
template class Socks5Ingress<pichi::test::Stream>;
#endif // BUILD_TEST

template <typename Stream>
static void writeString(Stream& stream, asio::yield_context yield, string_view str)
{
  auto len = static_cast<uint8_t>(str.size());
  write(stream, {&len, 1}, yield);
  write(stream, ConstBuffer<uint8_t>{str}, yield);
}

template <typename Stream> void Socks5Egress<Stream>::authenticate(Yield yield)
{
  static auto const VER = 0x01_u8;

  write(stream_, {&VER, 1}, yield);
  writeString(stream_, yield, credential_->first);
  writeString(stream_, yield, credential_->second);

  auto received = array<uint8_t, 2>{};
  read(stream_, received, yield);
  assertTrue(received.front() == 0x01_u8, PichiError::BAD_PROTO);
  assertTrue(received.back() == 0x00_u8, PichiError::BAD_PROTO);
}

template <typename Stream>
size_t Socks5Egress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(stream_, buf, yield);
}

template <typename Stream> void Socks5Egress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  write(stream_, buf, yield);
}

template <typename Stream> void Socks5Egress<Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <typename Stream> bool Socks5Egress<Stream>::readable() const { return isOpen(stream_); }

template <typename Stream> bool Socks5Egress<Stream>::writable() const { return isOpen(stream_); }

template <typename Stream>
void Socks5Egress<Stream>::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
{
  pichi::net::connect(next, stream_, yield);

  auto method = credential_.has_value() ? 0x02_u8 : 0x00_u8;
  auto buf = HeaderBuffer<uint8_t>{0x05, 0x01, method};

  write(stream_, {buf, 3}, yield);

  read(stream_, {buf, 2}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == method, PichiError::BAD_PROTO);

  if (credential_.has_value()) authenticate(yield);

  auto i = 0_sz;
  buf[i++] = 0x05;
  buf[i++] = 0x01;
  buf[i++] = 0x00;
  i += serializeEndpoint(remote, MutableBuffer<uint8_t>{buf} + i);
  write(stream_, {buf, i}, yield);

  read(stream_, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x00, PichiError::CONN_FAILURE,
             "Failed to establish connection with " + remote.host_ + ":" + remote.port_);
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);
  parseEndpoint([this, yield](auto dst) { read(stream_, dst, yield); });
}

template class Socks5Egress<tcp::socket>;

#ifdef ENABLE_TLS
template class Socks5Egress<ssl::stream<tcp::socket>>;
#endif // ENABLE_TLS

#ifdef BUILD_TEST
template class Socks5Egress<pichi::test::Stream>;
#endif // BUILD_TEST

} // namespace pichi::net
