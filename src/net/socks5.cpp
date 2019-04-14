#include "config.h"
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/asserts.hpp>
#include <pichi/common.hpp>
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

template <typename Stream>
size_t Socks5Adapter<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(stream_, buf, yield);
}

template <typename Stream> void Socks5Adapter<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  write(stream_, buf, yield);
}

template <typename Stream> void Socks5Adapter<Stream>::close() { pichi::net::close(stream_); }

template <typename Stream> bool Socks5Adapter<Stream>::readable() const { return isOpen(stream_); }

template <typename Stream> bool Socks5Adapter<Stream>::writable() const { return isOpen(stream_); }

template <typename Stream> Endpoint Socks5Adapter<Stream>::readRemote(Yield yield)
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

  uint8_t m = find(begin(buf), begin(buf) + len, 0x00) != begin(buf) + len ? 0x00 : 0xff;
  buf[0] = 0x05;
  buf[1] = m;
  write(stream_, {buf, 2}, yield);

  read(stream_, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x01, PichiError::BAD_PROTO);
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);

  return parseEndpoint([this, yield](auto dst) { read(stream_, dst, yield); });
}

template <typename Stream>
void Socks5Adapter<Stream>::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
{
  pichi::net::connect(next, stream_, yield);

  auto buf = HeaderBuffer<uint8_t>{0x05, 0x01, 0x00};

  write(stream_, {buf, 3}, yield);

  read(stream_, {buf, 2}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x00, PichiError::BAD_PROTO);

  auto i = 0_sz;
  buf[i++] = 0x05;
  buf[i++] = 0x01;
  buf[i++] = 0x00;
  i += serializeEndpoint(remote, {buf.data() + i, buf.size() - i});
  write(stream_, {buf, i}, yield);

  read(stream_, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x00, PichiError::CONN_FAILURE,
             "Failed to establish connection with " + remote.host_ + ":" + remote.port_);
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);
  parseEndpoint([this, yield](auto dst) { read(stream_, dst, yield); });
}

template <typename Stream> void Socks5Adapter<Stream>::confirm(Yield yield)
{
  static uint8_t const buf[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  write(stream_, buf, yield);
}

template <typename Stream> void Socks5Adapter<Stream>::disconnect(Yield yield)
{
  // REP = 0x04(Host unreachable) according to RFC1928
  static uint8_t const buf[] = {0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  auto ec = sys::error_code{};
  write(stream_, buf, yield[ec]);
}

template class Socks5Adapter<tcp::socket>;

#ifdef ENABLE_TLS
template class Socks5Adapter<ssl::stream<tcp::socket>>;
#endif // ENABLE_TLS

#ifdef BUILD_TEST
template class Socks5Adapter<pichi::test::Stream>;
#endif // BUILD_TEST

} // namespace pichi::net
