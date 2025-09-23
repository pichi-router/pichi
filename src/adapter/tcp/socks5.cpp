#include <pichi/common/config.hpp>
// Include config.hpp first
#include <algorithm>
#include <boost/asio/ip/tcp.hpp>
#include <format>
#include <pichi/adapter/tcp/socks5.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/error.hpp>
#include <pichi/common/literals.hpp>

namespace asio  = boost::asio;
namespace ip    = asio::ip;
namespace rngs  = std::ranges;
namespace sys   = boost::system;
namespace views = std::views;

namespace pichi::adapter::tcp {

namespace socks5 {

static ConstBuffer err_to_buf(sys::error_code const& ec)
{
#ifdef HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  static auto const NETWORK_UNREACHABLE = array{
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
  static auto const HOST_UNREACHABLE = array{
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
  static auto const CONNECTION_REFUSED = array{
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
  static auto const ADDRESS_TYPE_NOT_SUPPORTED = array{
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
  static auto const AUTH_FAILURE = array{0x01_u8, 0xff_u8};
  // static auto const AUTH_SUCCESS   = array{0x01_u8, 0x00_u8};
  static auto const METHOD_FAILURE = array{0x05_u8, 0xff_u8};
#else   // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  static uint8_t const NETWORK_UNREACHABLE[] =
      {0x05_u8, 0x03_u8, 0x00_u8, 0x01_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8};
  static uint8_t const HOST_UNREACHABLE[] =
      {0x05_u8, 0x04_u8, 0x00_u8, 0x01_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8};
  static uint8_t const CONNECTION_REFUSED[] =
      {0x05_u8, 0x05_u8, 0x00_u8, 0x01_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8};
  static uint8_t const ADDRESS_TYPE_NOT_SUPPORTED[] =
      {0x05_u8, 0x08_u8, 0x00_u8, 0x01_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8};
  static uint8_t const AUTH_FAILURE[] = {0x01_u8, 0xff_u8};
  // static uint8_t const AUTH_SUCCESS[]   = {0x01_u8, 0x00_u8};
  static uint8_t const METHOD_FAILURE[] = {0x05_u8, 0xff_u8};
#endif  // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION

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

template <typename Socket>
Socks5Ingress<Socket>::Socks5Ingress(vo::Ingress const& vo, Socket s)
  : stream_{
      vo.tls_.has_value()
      ? Stream{
          std::in_place_type<stream::Tls<Socket>>,
          stream::tls_context(*vo.tls_),
          std::move(s)
        }
      : Stream{std::move(s)}}, credential_{vo}
{
}

template <typename Socket> Awaitable<void> Socks5Ingress<Socket>::authenticate()
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
  co_await stream::read(stream_, {&ver, 1_sz});
  assertTrue(ver == 0x01_u8, PichiError::BAD_PROTO);

  auto u = co_await socks5::read_string(stream_);
  auto p = co_await socks5::read_string(stream_);
  assertTrue(credential_.authenticate(u, p), PichiError::UNAUTHENTICATED);

  /*
   * Response:
   * +----+--------+
   * |VER | STATUS |
   * +----+--------+
   * | 1  |   1    |
   * +----+--------+
   */
#ifdef HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  static auto const AUTH_SUCCESS = array{0x01_u8, 0x00_u8};
#else   // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  static uint8_t const AUTH_SUCCESS[] = {0x01_u8, 0x00_u8};
#endif  // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  co_await stream::write(stream_, AUTH_SUCCESS);
}

template <typename Socket> Awaitable<size_t> Socks5Ingress<Socket>::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(stream_, buf);
}

template <typename Socket> Awaitable<void> Socks5Ingress<Socket>::send(ConstBuffer buf)
{
  co_await stream::write(stream_, buf);
}

template <typename Socket> Awaitable<void> Socks5Ingress<Socket>::close()
{
  co_await redirect(stream::close(stream_));
}

template <typename Socket> Awaitable<Endpoint> Socks5Ingress<Socket>::read_remote()
{
  co_await stream::accept(stream_);

  auto buf = std::array<uint8_t, 512>{};
  co_await stream::read(stream_, {buf, 2});
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] > 0, PichiError::BAD_PROTO);

  auto len = buf[1];
  co_await stream::read(stream_, {buf, len});

  auto m = credential_.need_auth() ? 0x02_u8 : 0x00_u8;
  assertFalse(
      rngs::empty(ConstBuffer{buf, len} | views::filter([m](auto i) { return m == i; })),
      PichiError::BAD_AUTH_METHOD
  );
  buf[0] = 0x05;
  buf[1] = m;
  co_await stream::write(stream_, {buf, 2});

  if (credential_.need_auth()) co_await authenticate();

  co_await stream::read(stream_, {buf, 3});
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == 0x01, PichiError::BAD_PROTO);
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);

  co_return co_await parse_endpoint([this](auto buf) -> Awaitable<void> {
    co_await stream::read(stream_, buf);
  });
}

template <typename Socket> Awaitable<void> Socks5Ingress<Socket>::confirm()
{
#ifdef HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  static auto const CONFIRM = array{
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
#else   // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  static uint8_t const CONFIRM[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif  // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
  co_await stream::write(stream_, CONFIRM);
}

template <typename Socket>
Awaitable<void> Socks5Ingress<Socket>::disconnect(sys::error_code const& ec)
{
  co_await redirect(stream::write(stream_, socks5::err_to_buf(ec)));
}

template class Socks5Ingress<ip::tcp::socket>;

template <typename Socket>
Socks5Egress<Socket>::Socks5Egress(vo::Egress const& vo, IOExecutor const& ex)
  : stream_{
    vo.tls_.has_value()
    ? Stream{
        std::in_place_type<stream::Tls<Socket>>,
        stream::tls_context(*vo.tls_, vo.server_->host_),
        ex
      }
    : Stream{std::in_place_type<Socket>, ex}
  }, peer_{*vo.server_}, credential_{vo}
{
}

template <typename Socket> Awaitable<size_t> Socks5Egress<Socket>::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(stream_, buf);
}

template <typename Socket> Awaitable<void> Socks5Egress<Socket>::send(ConstBuffer buf)
{
  co_await stream::write(stream_, buf);
}

template <typename Socket> Awaitable<void> Socks5Egress<Socket>::close()
{
  co_await redirect(stream::close(stream_));
}

template <typename Stream> Awaitable<void> Socks5Egress<Stream>::connect(Endpoint const& remote)
{
  co_await stream::connect(stream_, peer_);

  auto buf = std::array<uint8_t, 512>{};
  auto m   = credential_.need_auth() ? 0x02_u8 : 0x00_u8;

  buf[0] = 0x05;
  buf[1] = 0x01;
  buf[2] = m;
  co_await stream::write(stream_, {buf, 3});

  co_await stream::read(stream_, {buf, 2});
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(buf[1] == m, PichiError::BAD_AUTH_METHOD);

  if (credential_.need_auth()) {
    co_await stream::write(stream_, credential_.data());

    co_await stream::read(stream_, {buf, 2});
    assertTrue(buf[0] == 0x01_u8, PichiError::BAD_PROTO);
    assertTrue(buf[1] == 0x00_u8, PichiError::UNAUTHENTICATED);
  }

  buf[0] = 0x05;
  buf[1] = 0x01;
  buf[2] = 0x00;
  co_await stream::write(stream_, {buf, serializeEndpoint(remote, MutableBuffer{buf} + 3) + 3});

  co_await stream::read(stream_, {buf, 3});
  assertTrue(buf[0] == 0x05, PichiError::BAD_PROTO);
  assertTrue(
      buf[1] == 0x00,
      PichiError::CONN_FAILURE,
      std::format("Failed to establish connection with {}:{}", remote.host_, remote.port_)
  );
  assertTrue(buf[2] == 0x00, PichiError::BAD_PROTO);
  co_await parse_endpoint([this](auto dst) { return stream::read(stream_, dst); });
}

template class Socks5Egress<ip::tcp::socket>;

}  // namespace pichi::adapter::tcp
