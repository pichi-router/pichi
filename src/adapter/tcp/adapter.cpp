#include <pichi/common/config.hpp>
// Include config.hpp first
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/crypto/key.hpp>

namespace asio = boost::asio;
namespace ip   = asio::ip;
namespace ssl  = asio::ssl;

using namespace std::literals;

namespace pichi::adapter::tcp {

template <CryptoMethod method> constexpr auto InPlaceType = std::in_place_type<SSAdapter<method>>;

template <typename R, typename... Args>
R create_ss_adapter(vo::ShadowsocksOption const& opt, Args&&... args)
{
  auto data = std::array<uint8_t, 1024>{};
  auto psk  = ConstBuffer{data, crypto::generateKey(opt.method_, opt.password_, data)};
  switch (opt.method_) {
  case CryptoMethod::AES_128_CTR:
    return R{InPlaceType<CryptoMethod::AES_128_CTR>, psk, std::forward<Args>(args)...};
  case CryptoMethod::AES_192_CTR:
    return R{InPlaceType<CryptoMethod::AES_192_CTR>, psk, std::forward<Args>(args)...};
  case CryptoMethod::AES_256_CTR:
    return R{InPlaceType<CryptoMethod::AES_256_CTR>, psk, std::forward<Args>(args)...};
  case CryptoMethod::AES_128_CFB:
    return R{InPlaceType<CryptoMethod::AES_128_CFB>, psk, std::forward<Args>(args)...};
  case CryptoMethod::AES_192_CFB:
    return R{InPlaceType<CryptoMethod::AES_192_CFB>, psk, std::forward<Args>(args)...};
  case CryptoMethod::AES_256_CFB:
    return R{InPlaceType<CryptoMethod::AES_256_CFB>, psk, std::forward<Args>(args)...};
  case CryptoMethod::CAMELLIA_128_CFB:
    return R{InPlaceType<CryptoMethod::CAMELLIA_128_CFB>, psk, std::forward<Args>(args)...};
  case CryptoMethod::CAMELLIA_192_CFB:
    return R{InPlaceType<CryptoMethod::CAMELLIA_192_CFB>, psk, std::forward<Args>(args)...};
  case CryptoMethod::CAMELLIA_256_CFB:
    return R{InPlaceType<CryptoMethod::CAMELLIA_256_CFB>, psk, std::forward<Args>(args)...};
  case CryptoMethod::CHACHA20:
    return R{InPlaceType<CryptoMethod::CHACHA20>, psk, std::forward<Args>(args)...};
  case CryptoMethod::SALSA20:
    return R{InPlaceType<CryptoMethod::SALSA20>, psk, std::forward<Args>(args)...};
  case CryptoMethod::CHACHA20_IETF:
    return R{InPlaceType<CryptoMethod::CHACHA20_IETF>, psk, std::forward<Args>(args)...};
  case CryptoMethod::AES_128_GCM:
    return R{InPlaceType<CryptoMethod::AES_128_GCM>, psk, std::forward<Args>(args)...};
  case CryptoMethod::AES_192_GCM:
    return R{InPlaceType<CryptoMethod::AES_192_GCM>, psk, std::forward<Args>(args)...};
  case CryptoMethod::AES_256_GCM:
    return R{InPlaceType<CryptoMethod::AES_256_GCM>, psk, std::forward<Args>(args)...};
  case CryptoMethod::CHACHA20_IETF_POLY1305:
    return R{InPlaceType<CryptoMethod::CHACHA20_IETF_POLY1305>, psk, std::forward<Args>(args)...};
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return R{InPlaceType<CryptoMethod::XCHACHA20_IETF_POLY1305>, psk, std::forward<Args>(args)...};
  default:
    fail();
  }
}

template <typename Socket> Ingress create_ingress(vo::Ingress const& vo, Socket s)
{
  switch (vo.type_) {
  case AdapterType::SS:
    assertTrue(vo.opt_.has_value());
    return create_ss_adapter<Ingress>(std::get<vo::ShadowsocksOption>(*vo.opt_), std::move(s));
  case AdapterType::SOCKS5:
    return vo.tls_.has_value()
               ? Ingress{std::in_place_type<Socks5Ingress<stream::Tls<Socket>>>, vo, std::move(s)}
               : Ingress{std::in_place_type<Socks5Ingress<Socket>>, vo, std::move(s)};
  case AdapterType::HTTP:
    return vo.tls_.has_value()
               ? Ingress{std::in_place_type<HttpIngress<stream::Tls<Socket>>>, vo, std::move(s)}
               : Ingress{std::in_place_type<HttpIngress<Socket>>, vo, std::move(s)};
  case AdapterType::TROJAN:
    return vo.websocket_.has_value()
               ? Ingress{std::in_place_type<TrojanIngress<stream::Websocket<stream::Tls<Socket>>>>, vo, std::move(s)}
               : Ingress{std::in_place_type<TrojanIngress<stream::Tls<Socket>>>, vo, std::move(s)};
  default:
    fail();
  }
}

Egress create_egress(vo::Egress const& vo, IOExecutor const& ex)
{
  switch (vo.type_) {
  case AdapterType::SS:
    assertTrue(vo.opt_.has_value());
    return create_ss_adapter<Egress>(std::get<vo::ShadowsocksOption>(*vo.opt_), ex);
  case AdapterType::DIRECT:
    return Egress{std::in_place_type<Direct>, ex};
  case AdapterType::REJECT:
    return Egress{std::in_place_type<RejectEgress>, vo, ex};
  case AdapterType::SOCKS5:
    return vo.tls_.has_value()
               ? Egress{std::in_place_type<Socks5Egress<stream::Tls<ip::tcp::socket>>>, vo, ex}
               : Egress{std::in_place_type<Socks5Egress<ip::tcp::socket>>, vo, ex};
  case AdapterType::HTTP:
    return vo.tls_.has_value()
               ? Egress{std::in_place_type<HttpEgress<stream::Tls<ip::tcp::socket>>>, vo, ex}
               : Egress{std::in_place_type<HttpEgress<ip::tcp::socket>>, vo, ex};
  case AdapterType::TROJAN:
    return vo.websocket_.has_value()
               ? Egress{std::in_place_type<TrojanEgress<stream::Websocket<stream::Tls<ip::tcp::socket>>>>, vo, ex}
               : Egress{std::in_place_type<TrojanEgress<stream::Tls<ip::tcp::socket>>>, vo, ex};
  default:
    fail();
  }
}

template Ingress create_ingress(vo::Ingress const&, ip::tcp::socket);

}  // namespace pichi::adapter::tcp
