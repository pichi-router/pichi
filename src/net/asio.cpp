#include <array>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/asio/write.hpp>
#include <pichi/api/rest.hpp>
#include <pichi/asserts.hpp>
#include <pichi/crypto/key.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/common.hpp>
#include <pichi/net/direct.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/http.hpp>
#include <pichi/net/socks5.hpp>
#include <pichi/net/ssaead.hpp>
#include <pichi/net/ssstream.hpp>

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
using ip::tcp;
using pichi::crypto::CryptoMethod;

namespace pichi::net {

template <typename Socket, typename Yield>
void connect(Endpoint const& endpoint, Socket& s, Yield yield)
{
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
void write(Socket& s, ConstBuffer<uint8_t> buf, Yield yield)
{
  asio::async_write(s, asio::buffer(buf), yield);
}

template void connect<tcp::socket, asio::yield_context>(Endpoint const&, tcp::socket&,
                                                        asio::yield_context);
template void read<tcp::socket, asio::yield_context>(tcp::socket&, MutableBuffer<uint8_t>,
                                                     asio::yield_context);
template void write<tcp::socket, asio::yield_context>(tcp::socket&, ConstBuffer<uint8_t>,
                                                      asio::yield_context);

template <typename Socket> unique_ptr<Inbound> makeInbound(api::InboundVO const& vo, Socket&& s)
{
  auto container = array<uint8_t, 1024>{0};
  auto psk = MutableBuffer<uint8_t>{container};
  switch (vo.type_) {
  case AdapterType::HTTP:
    return make_unique<HttpInbound>(forward<Socket>(s));
  case AdapterType::SOCKS5:
    return make_unique<Socks5Adapter>(forward<Socket>(s));
  case AdapterType::SS:
    psk = {container,
           generateKey(vo.method_.value(), ConstBuffer<uint8_t>{vo.password_.value()}, container)};
    switch (vo.method_.value()) {
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

template unique_ptr<Inbound> makeInbound<tcp::socket>(api::InboundVO const&, tcp::socket&&);

template <typename Socket> unique_ptr<Outbound> makeOutbound(api::OutboundVO const& vo, Socket&& s)
{
  auto container = array<uint8_t, 1024>{0};
  auto psk = MutableBuffer<uint8_t>{container};
  switch (vo.type_) {
  case AdapterType::HTTP:
    return make_unique<HttpOutbound>(forward<Socket>(s));
  case AdapterType::SOCKS5:
    return make_unique<Socks5Adapter>(forward<Socket>(s));
  case AdapterType::DIRECT:
    return make_unique<DirectAdapter>(forward<Socket>(s));
  case AdapterType::SS:
    psk = {container,
           generateKey(vo.method_.value(), ConstBuffer<uint8_t>{vo.password_.value()}, container)};
    switch (vo.method_.value()) {
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

template unique_ptr<Outbound> makeOutbound<tcp::socket>(api::OutboundVO const&, tcp::socket&&);

} // namespace pichi::net
