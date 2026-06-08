#include "pichi/common/config.hpp"
#include <algorithm>
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <format>
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/adapter/tcp/socks5.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/error.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/helpers.hpp>
#include <pichi/stream/test.hpp>

namespace asio = boost::asio;
namespace ip   = asio::ip;
namespace rngs = std::ranges;
namespace sys  = boost::system;

namespace pichi::adapter::tcp {

namespace socks5 {

static ConstBuffer err_to_buf(sys::error_code const& ec)
{
  static auto const NETWORK_UNREACHABLE = std::array{
      0x05_u8,
      0x03_u8,
      0x00_u8,
      0x01_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8
  };
  static auto const HOST_UNREACHABLE = std::array{
      0x05_u8,
      0x04_u8,
      0x00_u8,
      0x01_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8
  };
  static auto const CONNECTION_REFUSED = std::array{
      0x05_u8,
      0x05_u8,
      0x00_u8,
      0x01_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8
  };
  static auto const ADDRESS_TYPE_NOT_SUPPORTED = std::array{
      0x05_u8,
      0x08_u8,
      0x00_u8,
      0x01_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8
  };
  static auto const AUTH_FAILURE   = std::array{0x01_u8, 0xff_u8};
  static auto const METHOD_FAILURE = std::array{0x05_u8, 0xff_u8};

  if (ec == asio::error::address_family_not_supported) return ADDRESS_TYPE_NOT_SUPPORTED;
  if (ec == asio::error::connection_refused) return CONNECTION_REFUSED;
  if (ec == asio::error::connection_reset) return CONNECTION_REFUSED;
  if (ec == asio::error::host_not_found) return HOST_UNREACHABLE;
  if (ec == asio::error::host_unreachable) return HOST_UNREACHABLE;
  if (ec == asio::error::network_down) return NETWORK_UNREACHABLE;
  if (ec == asio::error::network_unreachable) return NETWORK_UNREACHABLE;
  if (ec == asio::error::timed_out) return HOST_UNREACHABLE;
  if (ec == PichiError::CONN_FAILURE) return HOST_UNREACHABLE;
  if (ec == PichiError::BAD_AUTH_METHOD) return METHOD_FAILURE;
  if (ec == PichiError::UNAUTHENTICATED) return AUTH_FAILURE;
  return {};
}

template <typename Stream> Awaitable<std::string> read_string(Stream& stream)
{
  auto buf = std::array<uint8_t, 256>{};
  co_await stream::read(stream, {buf, 1_sz});
  assertFalse(buf[0] == 0, PichiError::BAD_PROTO);

  auto len = buf[0];
  co_await stream::read(stream, {buf, len});
  co_return std::string{std::cbegin(buf), std::cbegin(buf) + len};
}

static auto gen_data(vo::Egress const& vo, MutableBuffer dst)
{
  if (!vo.credential_.has_value()) return 0_sz;

  auto& c = std::get<vo::UpEgressCredential>(*vo.credential_).credential_;
  auto  p = rngs::begin(dst);

  *p++ = 0x01;
  *p++ = static_cast<uint8_t>(c.first.size());
  rngs::copy(ConstBuffer{c.first}, p);
  p += c.first.size();
  *p++ = static_cast<uint8_t>(c.second.size());
  rngs::copy(ConstBuffer{c.second}, p);

  return 3_sz + c.first.size() + c.second.size();
}

IngressCredential::IngressCredential(vo::Ingress const& vo)
  : data_{
        vo.credential_.has_value() ? std::get<vo::UpIngressCredential>(*vo.credential_).credential_
                                   : std::unordered_map<std::string, std::string>{}
    }
{
}

bool IngressCredential::need_auth() const { return !data_.empty(); }

bool IngressCredential::authenticate(std::string const& u, std::string const& p) const
{
  auto it = data_.find(u);
  return it != std::cend(data_) && it->second == p;
}

EgressCredential::EgressCredential(vo::Egress const& vo) : len_{gen_data(vo, data_)} {}

bool EgressCredential::need_auth() const { return len_ > 0; }

ConstBuffer EgressCredential::data() const { return ConstBuffer{data_, len_}; }

}  // namespace socks5

template <stream::AsyncLayer NextLayer>
Socks5Ingress<NextLayer>::Socks5Ingress(vo::Ingress const& vo, NextLayer underlying)
  : underlying_{std::move(underlying)}, credential_{vo}
{
}

template <stream::AsyncLayer NextLayer> Awaitable<void> Socks5Ingress<NextLayer>::authenticate()
{
  /*
   * Request:
   * +----+------+----------+------+----------+
   * |VER | ULEN |  UNAME   | PLEN |  PASSWD  |
   * +----+------+----------+------+----------+
   * | 1  |  1   | 1 to 255 |  1   | 1 to 255 |
   * +----+------+----------+------+----------+
   */
  auto ver = 0_u8;
  co_await stream::read(underlying_, {&ver, 1_sz});
  assertTrue(ver == 0x01_u8, PichiError::BAD_PROTO);

  auto u = co_await socks5::read_string(underlying_);
  auto p = co_await socks5::read_string(underlying_);
  assertTrue(credential_.authenticate(u, p), PichiError::UNAUTHENTICATED);

  /*
   * Response:
   * +----+--------+
   * |VER | STATUS |
   * +----+--------+
   * | 1  |   1    |
   * +----+--------+
   */
  static auto const AUTH_SUCCESS = std::array{0x01_u8, 0x00_u8};
  co_await stream::write(underlying_, AUTH_SUCCESS);
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> Socks5Ingress<NextLayer>::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(underlying_, buf);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> Socks5Ingress<NextLayer>::send(ConstBuffer buf)
{
  co_await stream::write(underlying_, buf);
}

template <stream::AsyncLayer NextLayer> Awaitable<void> Socks5Ingress<NextLayer>::close()
{
  co_await redirect(stream::close(underlying_));
}

template <stream::AsyncLayer NextLayer>
Awaitable<Endpoint> Socks5Ingress<NextLayer>::continue_read_remote(ConstBuffer b)
{
  assertTrue(rngs::size(b) == 1);
  assertTrue(b[0] == 0x05_u8, PichiError::BAD_PROTO);

  auto buf = std::array<uint8_t, 512>{};
  co_await stream::read(underlying_, {buf, 1});
  assertTrue(buf[0] > 0, PichiError::BAD_PROTO);

  auto len = buf[0];
  co_await stream::read(underlying_, {buf, len});

  auto m = credential_.need_auth() ? 0x02_u8 : 0x00_u8;
  assertTrue(
      rngs::any_of(ConstBuffer{buf, len}, [m](auto b) { return b == m; }),
      PichiError::BAD_AUTH_METHOD
  );
  buf[0] = 0x05;
  buf[1] = m;
  co_await stream::write(underlying_, {buf, 2});

  if (credential_.need_auth()) co_await authenticate();

  co_await stream::read(underlying_, {buf, 3});
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x01, PichiError::BAD_PROTO);
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);

  co_return co_await parse_endpoint([this](auto buf) -> Awaitable<void> {
    co_await stream::read(underlying_, buf);
  });
}

template <stream::AsyncLayer NextLayer> Awaitable<Endpoint> Socks5Ingress<NextLayer>::read_remote()
{
  co_await stream::accept(underlying_);

  auto buf = std::array<uint8_t, 1>{};
  co_await stream::read(underlying_, buf);
  co_return co_await continue_read_remote(buf);
}

template <stream::AsyncLayer NextLayer> Awaitable<void> Socks5Ingress<NextLayer>::confirm()
{
  static auto const CONFIRM = std::array{
      0x05_u8,
      0x00_u8,
      0x00_u8,
      0x01_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8,
      0x00_u8
  };
  co_await stream::write(underlying_, CONFIRM);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> Socks5Ingress<NextLayer>::disconnect(sys::error_code const& ec)
{
  co_await redirect(stream::write(underlying_, socks5::err_to_buf(ec)));
}

template class Socks5Ingress<Socket>;
template class Socks5Ingress<Tls>;
template class Socks5Ingress<unit_test::TestSocket>;

template <stream::AsyncLayer NextLayer>
Socks5Egress<NextLayer>::Socks5Egress(vo::Egress const& vo, NextLayer underlying)
  : underlying_{std::move(underlying)}, peer_{*vo.server_}, credential_{vo}
{
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> Socks5Egress<NextLayer>::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(underlying_, buf);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> Socks5Egress<NextLayer>::send(ConstBuffer buf)
{
  co_await stream::write(underlying_, buf);
}

template <stream::AsyncLayer NextLayer> Awaitable<void> Socks5Egress<NextLayer>::close()
{
  co_await redirect(stream::close(underlying_));
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> Socks5Egress<NextLayer>::connect(Endpoint const& remote)
{
  co_await stream::connect(underlying_, peer_);

  auto buf = std::array<uint8_t, 512>{};
  auto m   = credential_.need_auth() ? 0x02_u8 : 0x00_u8;

  buf[0] = 0x05;
  buf[1] = 0x01;
  buf[2] = m;
  co_await stream::write(underlying_, {buf, 3});

  co_await stream::read(underlying_, {buf, 2});
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == m, PichiError::BAD_AUTH_METHOD);

  if (credential_.need_auth()) {
    co_await stream::write(underlying_, credential_.data());

    co_await stream::read(underlying_, {buf, 2});
    assertTrue(buf[0] == 0x01_u8, PichiError::BAD_PROTO);
    assertTrue(buf[1] == 0x00_u8, PichiError::UNAUTHENTICATED);
  }

  buf[0] = 0x05;
  buf[1] = 0x01;
  buf[2] = 0x00;
  co_await stream::write(underlying_, {buf, serializeEndpoint(remote, MutableBuffer{buf} + 3) + 3});

  co_await stream::read(underlying_, {buf, 3});
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(
      buf[1] == 0x00,
      PichiError::CONN_FAILURE,
      std::format("Failed to establish connection with {}:{}", remote.host_, remote.port_)
  );
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);
  co_await parse_endpoint([this](auto dst) { return stream::read(underlying_, dst); });
}

template class Socks5Egress<Socket>;
template class Socks5Egress<Tls>;
template class Socks5Egress<unit_test::TestSocket>;

}  // namespace pichi::adapter::tcp
