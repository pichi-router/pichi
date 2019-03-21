#include "config.h"
#include <array>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <pichi/api/rest.hpp>
#include <pichi/asserts.hpp>
#include <pichi/crypto/key.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/common.hpp>
#include <pichi/net/direct.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/http.hpp>
#include <pichi/net/reject.hpp>
#include <pichi/net/socks5.hpp>
#include <pichi/net/ssaead.hpp>
#include <pichi/net/ssstream.hpp>

#ifdef ENABLE_TLS
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#endif // ENABLE_TLS

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
namespace ssl = asio::ssl;
namespace sys = boost::system;
using ip::tcp;
using pichi::crypto::CryptoMethod;
using TcpSocket = tcp::socket;
using TlsSocket = ssl::stream<TcpSocket>;

namespace pichi::net {

#ifdef ENABLE_TLS
static auto createTlsContext(api::IngressVO const& vo)
{
  auto ctx = ssl::context{ssl::context::tls_server};
  ctx.use_certificate_chain_file(*vo.certFile_);
  ctx.use_private_key_file(*vo.keyFile_, ssl::context::pem);
  return ctx;
}

static auto createTlsContext(api::EgressVO const& vo)
{
  auto ctx = ssl::context{ssl::context::tls_client};
  if (*vo.insecure_) {
    ctx.set_verify_mode(ssl::context::verify_none);
  }
  else {
    ctx.set_verify_mode(ssl::context::verify_peer);
    ctx.set_default_verify_paths();
    if (vo.caFile_.has_value()) ctx.load_verify_file(*vo.caFile_);
  }
  return ctx;
}
#endif // ENABLE_TLS

template <typename Socket, typename Yield>
void connect(Endpoint const& endpoint, Socket& s, Yield yield)
{
#ifdef ENABLE_TLS
  if constexpr (IsSslStreamV<Socket>) {
    connect(endpoint, s.next_layer(), yield);
    s.async_handshake(ssl::stream_base::handshake_type::client, yield);
  }
  else
#endif // ENABLE_TLS
    asio::async_connect(s,
                        tcp::resolver{s.get_executor().context()}.async_resolve(
                            endpoint.host_, endpoint.port_, yield),
                        yield);
}

template <typename Socket, typename Yield>
void read(Socket& s, MutableBuffer<uint8_t> buf, Yield yield)
{
  asio::async_read(s, asio::buffer(buf), yield);
}

template <typename Socket, typename Yield>
size_t readSome(Socket& s, MutableBuffer<uint8_t> buf, Yield yield)
{
  return s.async_read_some(asio::buffer(buf), yield);
}

template <typename Socket, typename Yield>
void write(Socket& s, ConstBuffer<uint8_t> buf, Yield yield)
{
  asio::async_write(s, asio::buffer(buf), yield);
}

template <typename Socket> void close(Socket& s)
{
  if constexpr (IsSslStreamV<Socket>) {
    close(s.next_layer());
  }
  else {
    auto ec = sys::error_code{};
    s.close(ec);
  }
}

template <typename Socket> bool isOpen(Socket const& s)
{
  if constexpr (IsSslStreamV<Socket>) {
    return isOpen(s.next_layer());
  }
  else {
    return s.is_open();
  }
}

template <typename Socket> unique_ptr<Ingress> makeIngress(api::IngressVO const& vo, Socket&& s)
{
  auto container = array<uint8_t, 1024>{0};
  auto psk = MutableBuffer<uint8_t>{container};
  switch (vo.type_) {
  case AdapterType::HTTP:
#ifdef ENABLE_TLS
    if (*vo.tls_) {
      auto ctx = createTlsContext(vo);
      return make_unique<HttpIngress<TlsSocket>>(forward<Socket>(s), ctx);
    }
    else
#endif // ENABLE_TLS
      return make_unique<HttpIngress<TcpSocket>>(forward<Socket>(s));
  case AdapterType::SOCKS5:
#ifdef ENABLE_TLS
    if (*vo.tls_) {
      auto ctx = createTlsContext(vo);
      return make_unique<Socks5Adapter<TlsSocket>>(forward<Socket>(s), ctx);
    }
    else
#endif // ENABLE_TLS
      return make_unique<Socks5Adapter<TcpSocket>>(forward<Socket>(s));
  case AdapterType::SS:
    psk = {container, generateKey(*vo.method_, ConstBuffer<uint8_t>{*vo.password_}, container)};
    switch (*vo.method_) {
    case CryptoMethod::RC4_MD5:
      return make_unique<SSStreamAdapter<CryptoMethod::RC4_MD5>>(forward<Socket>(s), psk);
    case CryptoMethod::BF_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::BF_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_128_CTR:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CTR>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_192_CTR:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CTR>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_256_CTR:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CTR>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_128_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_192_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_256_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::CAMELLIA_128_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::CAMELLIA_192_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::CAMELLIA_256_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::CHACHA20:
      return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20>>(forward<Socket>(s), psk);
    case CryptoMethod::SALSA20:
      return make_unique<SSStreamAdapter<CryptoMethod::SALSA20>>(forward<Socket>(s), psk);
    case CryptoMethod::CHACHA20_IETF:
      return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20_IETF>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_128_GCM:
      return make_unique<SSAeadAdapter<CryptoMethod::AES_128_GCM>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_192_GCM:
      return make_unique<SSAeadAdapter<CryptoMethod::AES_192_GCM>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_256_GCM:
      return make_unique<SSAeadAdapter<CryptoMethod::AES_256_GCM>>(forward<Socket>(s), psk);
    case CryptoMethod::CHACHA20_IETF_POLY1305:
      return make_unique<SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305>>(forward<Socket>(s),
                                                                              psk);
    case CryptoMethod::XCHACHA20_IETF_POLY1305:
      return make_unique<SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305>>(forward<Socket>(s),
                                                                               psk);
    default:
      fail(PichiError::BAD_PROTO);
    }
  default:
    fail(PichiError::BAD_PROTO);
  }
}

template <typename Socket> unique_ptr<Egress> makeEgress(api::EgressVO const& vo, Socket&& s)
{
  auto container = array<uint8_t, 1024>{0};
  auto psk = MutableBuffer<uint8_t>{container};
  switch (vo.type_) {
  case AdapterType::HTTP:
#ifdef ENABLE_TLS
    if (*vo.tls_) {
      auto ctx = createTlsContext(vo);
      return make_unique<HttpEgress<TlsSocket>>(forward<Socket>(s), ctx);
    }
    else
#endif // ENABLE_TLS
      return make_unique<Socks5Adapter<TcpSocket>>(forward<Socket>(s));
  case AdapterType::SOCKS5:
#ifdef ENABLE_TLS
    if (*vo.tls_) {
      auto ctx = createTlsContext(vo);
      return make_unique<Socks5Adapter<ssl::stream<tcp::socket>>>(forward<Socket>(s), ctx);
    }
    else
#endif // ENABLE_TLS
      return make_unique<Socks5Adapter<tcp::socket>>(forward<Socket>(s));
  case AdapterType::DIRECT:
    return make_unique<DirectAdapter>(forward<Socket>(s));
  case AdapterType::REJECT:
    switch (*vo.mode_) {
    case api::DelayMode::RANDOM:
      return make_unique<RejectEgress>(forward<Socket>(s));
    case api::DelayMode::FIXED:
      return make_unique<RejectEgress>(forward<Socket>(s), *vo.delay_);
    default:
      fail(PichiError::BAD_PROTO);
    }
  case AdapterType::SS:
    psk = {container, generateKey(*vo.method_, ConstBuffer<uint8_t>{*vo.password_}, container)};
    switch (*vo.method_) {
    case CryptoMethod::RC4_MD5:
      return make_unique<SSStreamAdapter<CryptoMethod::RC4_MD5>>(forward<Socket>(s), psk);
    case CryptoMethod::BF_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::BF_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_128_CTR:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CTR>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_192_CTR:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CTR>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_256_CTR:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CTR>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_128_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_128_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_192_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_192_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_256_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::AES_256_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::CAMELLIA_128_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::CAMELLIA_192_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::CAMELLIA_256_CFB:
      return make_unique<SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB>>(forward<Socket>(s), psk);
    case CryptoMethod::CHACHA20:
      return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20>>(forward<Socket>(s), psk);
    case CryptoMethod::SALSA20:
      return make_unique<SSStreamAdapter<CryptoMethod::SALSA20>>(forward<Socket>(s), psk);
    case CryptoMethod::CHACHA20_IETF:
      return make_unique<SSStreamAdapter<CryptoMethod::CHACHA20_IETF>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_128_GCM:
      return make_unique<SSAeadAdapter<CryptoMethod::AES_128_GCM>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_192_GCM:
      return make_unique<SSAeadAdapter<CryptoMethod::AES_192_GCM>>(forward<Socket>(s), psk);
    case CryptoMethod::AES_256_GCM:
      return make_unique<SSAeadAdapter<CryptoMethod::AES_256_GCM>>(forward<Socket>(s), psk);
    case CryptoMethod::CHACHA20_IETF_POLY1305:
      return make_unique<SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305>>(forward<Socket>(s),
                                                                              psk);
    case CryptoMethod::XCHACHA20_IETF_POLY1305:
      return make_unique<SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305>>(forward<Socket>(s),
                                                                               psk);
    default:
      fail(PichiError::BAD_PROTO);
    }
  default:
    fail(PichiError::BAD_PROTO);
  }
}

using Yield = asio::yield_context;

template void connect<>(Endpoint const&, TcpSocket&, Yield);
template void read<>(TcpSocket&, MutableBuffer<uint8_t>, Yield);
template size_t readSome<>(TcpSocket&, MutableBuffer<uint8_t>, Yield);
template void write<>(TcpSocket&, ConstBuffer<uint8_t>, Yield);
template void close<>(TcpSocket&);
template bool isOpen<>(TcpSocket const&);

#ifdef ENABLE_TLS
template void connect<>(Endpoint const&, TlsSocket&, Yield);
template void read<>(TlsSocket&, MutableBuffer<uint8_t>, Yield);
template size_t readSome<>(TlsSocket&, MutableBuffer<uint8_t>, Yield);
template void write<>(TlsSocket&, ConstBuffer<uint8_t>, Yield);
template void close<>(TlsSocket&);
template bool isOpen<>(TlsSocket const&);
#endif // ENABLE_TLS

template unique_ptr<Ingress> makeIngress<>(api::IngressVO const&, TcpSocket&&);
template unique_ptr<Egress> makeEgress<>(api::EgressVO const&, TcpSocket&&);

} // namespace pichi::net
