#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/constants.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/net/helper.hpp>
#include <pichi/net/ssstream.hpp>
#include <pichi/stream/test.hpp>
#include <utility>

using namespace std;
using namespace pichi::crypto;
using boost::asio::ip::tcp;

namespace pichi::net {

static size_t const MAX_HEADER_SIZE = 512;
using HeaderBuffer = array<uint8_t, MAX_HEADER_SIZE>;
using FrameBuffer = array<uint8_t, MAX_FRAME_SIZE>;

template <CryptoMethod method, typename Stream>
size_t SSStreamAdapter<method, Stream>::recv(MutableBuffer plain, Yield yield)
{
  if (!ivReceived_) {
    auto iv = array<uint8_t, IV_SIZE<method>>{};
    readIV(iv, yield);
  }

  auto cipher = FrameBuffer{};
  auto len = readSome(stream_, {cipher, plain.size()}, yield);
  return decryptor_.decrypt({cipher, len}, plain);
}

template <CryptoMethod method, typename Stream>
void SSStreamAdapter<method, Stream>::send(ConstBuffer plain, Yield yield)
{
  if (!ivSent_) {
    write(stream_, encryptor_.getIv(), yield);
    ivSent_ = true;
  }

  auto cipher = FrameBuffer{};
  while (plain.size() > 0) {
    auto consumed = min(plain.size(), cipher.size());
    write(stream_, {cipher, encryptor_.encrypt({plain, consumed}, {cipher, consumed})}, yield);
    plain += consumed;
  }
}

template <CryptoMethod method, typename Stream>
void SSStreamAdapter<method, Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <CryptoMethod method, typename Stream>
bool SSStreamAdapter<method, Stream>::readable() const
{
  return stream_.is_open();
}

template <CryptoMethod method, typename Stream>
bool SSStreamAdapter<method, Stream>::writable() const
{
  return stream_.is_open();
}

template <CryptoMethod method, typename Stream>
size_t SSStreamAdapter<method, Stream>::readIV(MutableBuffer iv, Yield yield)
{
  assertFalse(ivReceived_);
  assertTrue(iv.size() >= IV_SIZE<method>);
  read(stream_, {iv, IV_SIZE<method>}, yield);
  decryptor_.setIv({iv, IV_SIZE<method>});
  ivReceived_ = true;
  return IV_SIZE<method>;
}

template <CryptoMethod method, typename Stream>
Endpoint SSStreamAdapter<method, Stream>::readRemote(Yield yield)
{
  return parseEndpoint([this, yield](auto dst) { recv(dst, yield); });
}

template <CryptoMethod method, typename Stream> void SSStreamAdapter<method, Stream>::confirm(Yield)
{
}

template <CryptoMethod method, typename Stream>
void SSStreamAdapter<method, Stream>::connect(Endpoint const& remote, ResolveResults next,
                                              Yield yield)
{
  pichi::net::connect(next, stream_, yield);

  auto plain = HeaderBuffer{};
  auto plen = serializeEndpoint(remote, plain);

  send({plain, plen}, yield);
}

#if MBEDTLS_VERSION_MAJOR < 3
template class SSStreamAdapter<CryptoMethod::RC4_MD5, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::BF_CFB, tcp::socket>;
#endif  // MBEDTLS_VERSION_MAJOR < 3
template class SSStreamAdapter<CryptoMethod::AES_128_CTR, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::AES_192_CTR, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::AES_256_CTR, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::AES_128_CFB, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::AES_192_CFB, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::AES_256_CFB, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::CHACHA20, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::SALSA20, tcp::socket>;
template class SSStreamAdapter<CryptoMethod::CHACHA20_IETF, tcp::socket>;

#ifdef BUILD_TEST
#if MBEDTLS_VERSION_MAJOR < 3
template class SSStreamAdapter<CryptoMethod::RC4_MD5, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::BF_CFB, stream::TestStream>;
#endif  // MBEDTLS_VERSION_MAJOR < 3
template class SSStreamAdapter<CryptoMethod::AES_128_CTR, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::AES_192_CTR, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::AES_256_CTR, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::AES_128_CFB, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::AES_192_CFB, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::AES_256_CFB, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::CHACHA20, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::SALSA20, stream::TestStream>;
template class SSStreamAdapter<CryptoMethod::CHACHA20_IETF, stream::TestStream>;
#endif  // BUILD_TEST

}  // namespace pichi::net
