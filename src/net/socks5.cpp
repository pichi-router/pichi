#include <array>
#include <pichi/asserts.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/socks5.hpp>
#include <utility>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;

namespace pichi::net {

template <typename T> using HeaderBuffer = array<T, 512>;

Socks5Adapter::Socks5Adapter(Socket&& socket) : socket_{move(socket)} {}

size_t Socks5Adapter::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return socket_.async_read_some(asio::buffer(buf), yield);
}

void Socks5Adapter::send(ConstBuffer<uint8_t> buf, Yield yield) { write(socket_, buf, yield); }

void Socks5Adapter::close() { pichi::net::close(socket_); }

bool Socks5Adapter::readable() const { return socket_.is_open(); }

bool Socks5Adapter::writable() const { return socket_.is_open(); }

Endpoint Socks5Adapter::readRemote(Yield yield)
{
  auto buf = HeaderBuffer<uint8_t>{};

  read(socket_, {buf, 2}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] > 0, PichiError::BAD_PROTO);

  uint8_t len = buf[1];
  read(socket_, {buf, len}, yield);

  uint8_t m = find(begin(buf), begin(buf) + len, 0x00) != end(buf) ? 0x00 : 0xff;
  buf[0] = 0x05;
  buf[1] = m;
  write(socket_, {buf, 2}, yield);

  read(socket_, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x01, PichiError::BAD_PROTO);
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);

  return parseEndpoint([this, yield](auto dst) { read(socket_, dst, yield); });
}

void Socks5Adapter::confirm(Yield yield)
{
  static uint8_t const buf[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  write(socket_, buf, yield);
}

void Socks5Adapter::disconnect(Yield yield)
{
  // REP = 0x04(Host unreachable) according to RFC1928
  static uint8_t const buf[] = {0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  auto ec = sys::error_code{};
  write(socket_, buf, yield[ec]);
}

void Socks5Adapter::connect(Endpoint const& remote, Endpoint const& next, Yield yield)
{
  auto buf = HeaderBuffer<uint8_t>{0x05, 0x01, 0x00};

  pichi::net::connect(next, socket_, yield);

  write(socket_, {buf, 3}, yield);

  read(socket_, {buf, 2}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x00, PichiError::BAD_PROTO);

  size_t i = 0;
  buf[i++] = 0x05;
  buf[i++] = 0x01;
  buf[i++] = 0x00;
  i += serializeEndpoint(remote, {buf.data() + i, buf.size() - i});
  write(socket_, {buf, i}, yield);

  read(socket_, {buf, 3}, yield);
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x00, PichiError::CONN_FAILURE,
             "Failed to establish connection with " + remote.host_ + ":" + remote.port_);
  parseEndpoint([this, yield](auto dst) { read(socket_, dst, yield); });
}

} // namespace pichi::net
