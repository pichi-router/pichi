#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <pichi/asserts.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/socks5.hpp>
#include <utility>

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
  if constexpr (IsSslStreamV<Stream>) {
    stream_.async_handshake(ssl::stream_base::handshake_type::server, yield);
  }

  auto buf = HeaderBuffer<uint8_t>{};

  read(stream_, {buf, 2}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] > 0, PichiError::BAD_PROTO);

  uint8_t len = buf[1];
  read(stream_, {buf, len}, yield);

  uint8_t m = find(begin(buf), begin(buf) + len, 0x00) != end(buf) ? 0x00 : 0xff;
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

  size_t i = 0;
  buf[i++] = 0x05;
  buf[i++] = 0x01;
  buf[i++] = 0x00;
  i += serializeEndpoint(remote, {buf.data() + i, buf.size() - i});
  write(stream_, {buf, i}, yield);

  read(stream_, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x00, PichiError::CONN_FAILURE,
             "Failed to establish connection with " + remote.host_ + ":" + remote.port_);
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
template class Socks5Adapter<ssl::stream<tcp::socket>>;

/*
namespace detail {

template <typename T> using HeaderBuffer = array<T, 512>;
using Yield = Adapter::Yield;

template <typename Stream> static Endpoint readRemote(Stream& stream, Yield yield)
{
  auto buf = HeaderBuffer<uint8_t>{};

  read(stream, {buf, 2}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] > 0, PichiError::BAD_PROTO);

  uint8_t len = buf[1];
  read(stream, {buf, len}, yield);

  uint8_t m = find(begin(buf), begin(buf) + len, 0x00) != end(buf) ? 0x00 : 0xff;
  buf[0] = 0x05;
  buf[1] = m;
  write(stream, {buf, 2}, yield);

  read(stream, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x01, PichiError::BAD_PROTO);
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);

  return parseEndpoint([&stream, yield](auto dst) { read(stream, dst, yield); });
}

template <typename Stream> static void confirm(Stream& stream, Yield yield)
{
  static uint8_t const buf[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  write(stream, buf, yield);
}

template <typename Stream> static void disconnect(Stream& stream, Yield yield)
{
  // REP = 0x04(Host unreachable) according to RFC1928
  static uint8_t const buf[] = {0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  auto ec = sys::error_code{};
  write(stream, buf, yield[ec]);
}

template <typename Stream>
static void connect(Endpoint const& remote, Endpoint const& next, Stream& stream, Yield yield)
{
  auto buf = HeaderBuffer<uint8_t>{0x05, 0x01, 0x00};

  write(stream, {buf, 3}, yield);

  read(stream, {buf, 2}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x00, PichiError::BAD_PROTO);

  size_t i = 0;
  buf[i++] = 0x05;
  buf[i++] = 0x01;
  buf[i++] = 0x00;
  i += serializeEndpoint(remote, {buf.data() + i, buf.size() - i});
  write(stream, {buf, i}, yield);

  read(stream, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x00, PichiError::CONN_FAILURE,
             "Failed to establish connection with " + remote.host_ + ":" + remote.port_);
  parseEndpoint([&stream, yield](auto dst) { read(stream, dst, yield); });
}

} // namespace detail

Socks5Adapter::Socks5Adapter(Socket&& socket) : socket_{move(socket)} {}

size_t Socks5Adapter::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(socket_, buf, yield);
}

void Socks5Adapter::send(ConstBuffer<uint8_t> buf, Yield yield) { write(socket_, buf, yield); }

void Socks5Adapter::close() { pichi::net::close(socket_); }

bool Socks5Adapter::readable() const { return isOpen(socket_); }

bool Socks5Adapter::writable() const { return isOpen(socket_); }

Endpoint Socks5Adapter::readRemote(Yield yield) { return detail::readRemote(socket_, yield); }

void Socks5Adapter::confirm(Yield yield) { detail::confirm(socket_, yield); }

void Socks5Adapter::disconnect(Yield yield) { detail::disconnect(socket_, yield); }

void Socks5Adapter::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
{
  pichi::net::connect(next, socket_, yield);
  detail::connect(remote, next, socket_, yield);
}

Socks5sIngress::Socks5sIngress(Socket&& s, boost::asio::ssl::context& ctx) : stream_{move(s), ctx}
{
}

size_t Socks5sIngress::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(stream_, buf, yield);
}

void Socks5sIngress::send(ConstBuffer<uint8_t> buf, Yield yield) { write(stream_, buf, yield); }

void Socks5sIngress::close() { pichi::net::close(stream_); }

bool Socks5sIngress::readable() const { return isOpen(stream_); }

bool Socks5sIngress::writable() const { return isOpen(stream_); }

Endpoint Socks5sIngress::readRemote(Yield yield)
{
  stream_.async_handshake(Stream::server, yield);
  return detail::readRemote(stream_, yield);
}

void Socks5sIngress::confirm(Yield yield) { detail::confirm(stream_, yield); }

void Socks5sIngress::disconnect(Yield yield) { detail::disconnect(stream_, yield); }

Socks5sEgress::Socks5sEgress(Socket&& s, boost::asio::ssl::context& ctx) : stream_{move(s), ctx} {}

size_t Socks5sEgress::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(stream_, buf, yield);
}

void Socks5sEgress::send(ConstBuffer<uint8_t> buf, Yield yield) { write(stream_, buf, yield); }

void Socks5sEgress::close() { pichi::net::close(stream_); }

bool Socks5sEgress::readable() const { return isOpen(stream_); }

bool Socks5sEgress::writable() const { return isOpen(stream_); }

void Socks5sEgress::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
{
  pichi::net::connect(next, stream_, yield);
  detail::connect(remote, next, stream_, yield);
}
*/

} // namespace pichi::net
