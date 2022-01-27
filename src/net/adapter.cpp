#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <functional>
#include <optional>
#include <pichi/api/ingress_holder.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/crypto/key.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/direct.hpp>
#include <pichi/net/http.hpp>
#include <pichi/net/reject.hpp>
#include <pichi/net/socks5.hpp>
#include <pichi/net/ssaead.hpp>
#include <pichi/net/ssstream.hpp>
#include <pichi/net/trojan.hpp>
#include <pichi/net/tunnel.hpp>
#include <pichi/stream/tls.hpp>
#include <pichi/stream/websocket.hpp>
#include <pichi/vo/credential.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/messages.hpp>
#include <pichi/vo/options.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION >= 107300
#include <boost/asio/ssl/host_name_verification.hpp>
#else  // BOOST_VERSION >= 107300
#include <boost/asio/ssl/rfc2818_verification.hpp>
#endif  // BOOST_VERSION >= 107300

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
namespace ssl = asio::ssl;

namespace pichi::net {

using TCPSocket = asio::ip::tcp::socket;
using TlsStream = stream::TlsStream<TCPSocket>;
using WsStream = stream::WsStream<TCPSocket>;
using WssStream = stream::WsStream<TlsStream>;

static auto createTlsContext(vo::TlsIngressOption const& option)
{
  auto ctx = ssl::context{ssl::context::tls_server};
  ctx.use_certificate_chain_file(option.certFile_);
  ctx.use_private_key_file(option.keyFile_, ssl::context::pem);
  return ctx;
}

static auto createTlsContext(vo::TlsEgressOption const& option, string const& serverName)
{
  // TODO SNI not supported yet
  auto ctx = ssl::context{ssl::context::tls_client};
  if (option.insecure_) {
    ctx.set_verify_mode(ssl::context::verify_none);
    return ctx;
  }

  ctx.set_verify_mode(ssl::context::verify_peer);
  if (option.caFile_.has_value())
    ctx.load_verify_file(*option.caFile_);
  else {
    ctx.set_default_verify_paths();
#if BOOST_VERSION >= 107300
    ctx.set_verify_callback(ssl::host_name_verification{option.serverName_.value_or(serverName)});
#else   // BOOST_VERSION >= 107300
    ctx.set_verify_callback(ssl::rfc2818_verification{option.serverName_.value_or(serverName)});
#endif  // BOOST_VERSION >= 107300
  }
  return ctx;
}

static optional<function<bool(string const&, string const&)>>
    genAuthenticator(optional<vo::Ingress::Credential> const& credential)
{
  if (credential.has_value())
    return [&all = get<vo::UpIngressCredential>(*credential).credential_](auto&& u, auto&& p) {
      auto it = all.find(u);
      return it != cend(all) && it->second == p;
    };
  return {};
}

template <typename Socket>
unique_ptr<Ingress> makeShadowsocksIngress(Socket&& s, vo::ShadowsocksOption const& option)
{
  auto container = array<uint8_t, 1024>{0};
  auto psk = MutableBuffer<uint8_t>{container};
  psk = {container,
         crypto::generateKey(option.method_, ConstBuffer<uint8_t>{option.password_}, container)};
  switch (option.method_) {
#if MBEDTLS_VERSION_MAJOR < 3
  case CryptoMethod::RC4_MD5:
    return make_unique<SSStreamAdapter<CryptoMethod::RC4_MD5, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::BF_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::BF_CFB, Socket>>(psk, forward<Socket>(s));
#else   // MBEDTLS_VERSION_MAJOR < 3
  case CryptoMethod::RC4_MD5:
  case CryptoMethod::BF_CFB:
    fail(PichiError::SEMANTIC_ERROR, vo::msg::DEPRECATED_METHOD);
#endif  // MBEDTLS_VERSION_MAJOR < 3
  case CryptoMethod::AES_128_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CTR, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_192_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CTR, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_256_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CTR, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_128_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CFB, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_192_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CFB, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_256_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CFB, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::CAMELLIA_128_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB, Socket>>(psk,
                                                                                forward<Socket>(s));
  case CryptoMethod::CAMELLIA_192_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB, Socket>>(psk,
                                                                                forward<Socket>(s));
  case CryptoMethod::CAMELLIA_256_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB, Socket>>(psk,
                                                                                forward<Socket>(s));
  case CryptoMethod::CHACHA20:
    return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::SALSA20:
    return make_unique<SSStreamAdapter<CryptoMethod::SALSA20, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::CHACHA20_IETF:
    return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20_IETF, Socket>>(psk,
                                                                             forward<Socket>(s));
  case CryptoMethod::AES_128_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_128_GCM, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_192_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_192_GCM, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::AES_256_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_256_GCM, Socket>>(psk, forward<Socket>(s));
  case CryptoMethod::CHACHA20_IETF_POLY1305:
    return make_unique<SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305, Socket>>(
        psk, forward<Socket>(s));
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return make_unique<SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305, Socket>>(
        psk, forward<Socket>(s));
  default:
    fail(PichiError::BAD_PROTO);
  }
}

static unique_ptr<Egress> makeShadowsocksEgress(vo::ShadowsocksOption const& option,
                                                asio::io_context& io)
{
  auto container = array<uint8_t, 1024>{};
  auto len = crypto::generateKey(option.method_, ConstBuffer<uint8_t>{option.password_}, container);
  auto psk = MutableBuffer<uint8_t>{container, len};

  switch (option.method_) {
#if MBEDTLS_VERSION_MAJOR < 3
  case CryptoMethod::RC4_MD5:
    return make_unique<SSStreamAdapter<CryptoMethod::RC4_MD5, TCPSocket>>(psk, io);
  case CryptoMethod::BF_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::BF_CFB, TCPSocket>>(psk, io);
#else   // MBEDTLS_VERSION_MAJOR < 3
  case CryptoMethod::RC4_MD5:
  case CryptoMethod::BF_CFB:
    fail(PichiError::SEMANTIC_ERROR, vo::msg::DEPRECATED_METHOD);
#endif  // MBEDTLS_VERSION_MAJOR < 3
  case CryptoMethod::AES_128_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CTR, TCPSocket>>(psk, io);
  case CryptoMethod::AES_192_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CTR, TCPSocket>>(psk, io);
  case CryptoMethod::AES_256_CTR:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CTR, TCPSocket>>(psk, io);
  case CryptoMethod::AES_128_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CFB, TCPSocket>>(psk, io);
  case CryptoMethod::AES_192_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CFB, TCPSocket>>(psk, io);
  case CryptoMethod::AES_256_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CFB, TCPSocket>>(psk, io);
  case CryptoMethod::CAMELLIA_128_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB, TCPSocket>>(psk, io);
  case CryptoMethod::CAMELLIA_192_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB, TCPSocket>>(psk, io);
  case CryptoMethod::CAMELLIA_256_CFB:
    return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB, TCPSocket>>(psk, io);
  case CryptoMethod::CHACHA20:
    return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20, TCPSocket>>(psk, io);
  case CryptoMethod::SALSA20:
    return make_unique<SSStreamAdapter<CryptoMethod::SALSA20, TCPSocket>>(psk, io);
  case CryptoMethod::CHACHA20_IETF:
    return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20_IETF, TCPSocket>>(psk, io);
  case CryptoMethod::AES_128_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_128_GCM, TCPSocket>>(psk, io);
  case CryptoMethod::AES_192_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_192_GCM, TCPSocket>>(psk, io);
  case CryptoMethod::AES_256_GCM:
    return make_unique<SSAeadAdapter<CryptoMethod::AES_256_GCM, TCPSocket>>(psk, io);
  case CryptoMethod::CHACHA20_IETF_POLY1305:
    return make_unique<SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305, TCPSocket>>(psk, io);
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return make_unique<SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305, TCPSocket>>(psk, io);
  default:
    fail(PichiError::BAD_PROTO);
  }
}

template <template <typename> typename Ingress, typename Socket>
unique_ptr<pichi::Ingress> makeHttpOrSocks5Ingress(vo::Ingress const& vo, Socket&& s)
{
  if (vo.tls_.has_value())
    return make_unique<Ingress<TlsStream>>(genAuthenticator(vo.credential_),
                                           createTlsContext(*vo.tls_), forward<Socket>(s));
  else
    return make_unique<Ingress<TCPSocket>>(genAuthenticator(vo.credential_), forward<Socket>(s));
}

template <template <typename> typename Egress>
unique_ptr<pichi::Egress> makeHttpOrSocks5Egress(vo::Egress const& vo, asio::io_context& io)
{
  using NoCredential = optional<pair<string, string>>;
  auto cred = vo.credential_.has_value() ? get<vo::UpEgressCredential>(*vo.credential_).credential_
                                         : NoCredential{};
  if (vo.tls_.has_value())
    return make_unique<Egress<TlsStream>>(move(cred), vo.tls_->sni_,
                                          createTlsContext(*vo.tls_, vo.server_->host_), io);
  else
    return make_unique<Egress<TCPSocket>>(move(cred), io);
}

static auto makeRejectEgress(vo::RejectOption const& option, asio::io_context& io)
{
  switch (option.mode_) {
  case DelayMode::RANDOM:
    return make_unique<RejectEgress>(io);
  case DelayMode::FIXED:
    return make_unique<RejectEgress>(io, *option.delay_);
  default:
    fail(PichiError::BAD_PROTO);
  }
}

unique_ptr<Ingress> makeIngress(api::IngressHolder& holder, TCPSocket&& s)
{
  auto& vo = holder.vo_;
  switch (vo.type_) {
  case AdapterType::TROJAN:
    if (vo.websocket_)
      return make_unique<TrojanIngress<WssStream>>(
          get<vo::TrojanOption>(*vo.opt_).remote_,
          cbegin(get<vo::TrojanIngressCredential>(*vo.credential_).credential_),
          cend(get<vo::TrojanIngressCredential>(*vo.credential_).credential_), vo.websocket_->path_,
          vo.websocket_->host_.value_or(vo.bind_.front().host_), createTlsContext(*vo.tls_),
          move(s));
    else
      return make_unique<TrojanIngress<TlsStream>>(
          get<vo::TrojanOption>(*vo.opt_).remote_,
          cbegin(get<vo::TrojanIngressCredential>(*vo.credential_).credential_),
          cend(get<vo::TrojanIngressCredential>(*vo.credential_).credential_),
          createTlsContext(*vo.tls_), move(s));
  case AdapterType::HTTP:
    return makeHttpOrSocks5Ingress<HttpIngress>(vo, move(s));
  case AdapterType::SOCKS5:
    return makeHttpOrSocks5Ingress<Socks5Ingress>(vo, move(s));
  case AdapterType::SS:
    return makeShadowsocksIngress(move(s), get<vo::ShadowsocksOption>(*vo.opt_));
  case AdapterType::TUNNEL:
    return make_unique<TunnelIngress<TCPSocket>>(holder.data_, move(s));
  case AdapterType::VMESS:
    fail(PichiError::SEMANTIC_ERROR, vo::msg::NOT_IMPLEMENTED);
  default:
    fail(PichiError::BAD_PROTO);
  }
}

unique_ptr<Egress> makeEgress(vo::Egress const& vo, asio::io_context& io)
{
  switch (vo.type_) {
  case AdapterType::TROJAN:
    if (vo.websocket_)
      return make_unique<TrojanEgress<WssStream>>(
          get<vo::TrojanEgressCredential>(*vo.credential_).credential_, vo.websocket_->path_,
          vo.websocket_->host_.value_or(vo.server_->host_), vo.tls_->sni_,
          createTlsContext(*vo.tls_, vo.server_->host_), io);
    else
      return make_unique<TrojanEgress<TlsStream>>(
          get<vo::TrojanEgressCredential>(*vo.credential_).credential_, vo.tls_->sni_,
          createTlsContext(*vo.tls_, vo.server_->host_), io);
  case AdapterType::HTTP:
    return makeHttpOrSocks5Egress<HttpEgress>(vo, io);
  case AdapterType::SOCKS5:
    return makeHttpOrSocks5Egress<Socks5Egress>(vo, io);
  case AdapterType::DIRECT:
    return make_unique<DirectAdapter>(io);
  case AdapterType::REJECT:
    return makeRejectEgress(get<vo::RejectOption>(*vo.opt_), io);
  case AdapterType::SS:
    return makeShadowsocksEgress(get<vo::ShadowsocksOption>(*vo.opt_), io);
  case AdapterType::VMESS:
    fail(PichiError::SEMANTIC_ERROR, vo::msg::NOT_IMPLEMENTED);
  default:
    fail(PichiError::BAD_PROTO);
  }
}

}  // namespace pichi::net