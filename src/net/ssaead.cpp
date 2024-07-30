#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/constants.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/net/helper.hpp>
#include <pichi/net/ssaead.hpp>
#include <pichi/stream/test.hpp>
#include <utility>

using namespace std;
using namespace pichi::crypto;
namespace asio = boost::asio;
using asio::ip::tcp;

namespace pichi::net {

template <CryptoMethod method, typename Stream>
void SSAeadAdapter<method, Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <CryptoMethod method, typename Stream> bool SSAeadAdapter<method, Stream>::readable() const
{
  return cache_.size() > 0 || stream_.is_open();
}

template <CryptoMethod method, typename Stream> bool SSAeadAdapter<method, Stream>::writable() const
{
  return stream_.is_open();
}

template <CryptoMethod method, typename Stream>
size_t SSAeadAdapter<method, Stream>::recv(MutableBuffer plain, Yield yield)
{
  if (!ivReceived_) {
    auto iv = array<uint8_t, IV_SIZE<method>>{};
    readIV(iv, yield);
  }

  // Here's some buffer who should be returned first.
  if (cache_.size() > 0) return copyTo(plain);

  auto len = recvFrame(plain, yield);

  // frame is cached if plain's size is less than this frame,
  // otherwise, frame is written into plain directly.
  return cache_.size() == 0 ? len : copyTo(plain);
}

template <CryptoMethod method, typename Stream>
void SSAeadAdapter<method, Stream>::send(ConstBuffer plain, Yield yield)
{
  if (!ivSent_) {
    write(stream_, encryptor_.getIv(), yield);
    ivSent_ = true;
  }

  using CipherBuffer = array<uint8_t, 2 + MAX_FRAME_SIZE + 2 * TAG_SIZE<method>>;

  auto cipher = CipherBuffer{};
  auto len = encrypt(plain, cipher);

  write(stream_, {cipher, len}, yield);
}

template <CryptoMethod method, typename Stream>
void SSAeadAdapter<method, Stream>::connect(Endpoint const& remote, ResolveResults next,
                                            Yield yield)
{
  pichi::net::connect(next, stream_, yield);

  auto plain = array<uint8_t, 512>{};
  auto plen = serializeEndpoint(remote, plain);

  send({plain, plen}, yield);
}

template <CryptoMethod method, typename Stream>
size_t SSAeadAdapter<method, Stream>::readIV(MutableBuffer iv, Yield yield)
{
  assertFalse(ivReceived_);
  assertTrue(iv.size() >= IV_SIZE<method>);
  read(stream_, {iv, IV_SIZE<method>}, yield);
  decryptor_.setIv({iv, IV_SIZE<method>});
  ivReceived_ = true;
  return IV_SIZE<method>;
}

template <CryptoMethod method, typename Stream>
Endpoint SSAeadAdapter<method, Stream>::readRemote(Yield yield)
{
  return parseEndpoint([this, yield](auto dst) {
    for (size_t r = 0; r < dst.size(); r += recv(dst + r, yield))
      ;
  });
}

template <CryptoMethod method, typename Stream> void SSAeadAdapter<method, Stream>::confirm(Yield)
{
}

template <CryptoMethod method, typename Stream>
MutableBuffer SSAeadAdapter<method, Stream>::prepare(size_t n, MutableBuffer provided)
{
  if (n <= provided.size()) return {provided, n};
  auto buf = cache_.prepare(n);
  cache_.commit(n);
  return buf;
}

template <CryptoMethod method, typename Stream>
void SSAeadAdapter<method, Stream>::recvBlock(MutableBuffer block, Yield yield)
{
  using CipherBuffer = array<uint8_t, MAX_FRAME_SIZE + TAG_SIZE<method>>;

  auto cipher = CipherBuffer{};
  auto clen = block.size() + TAG_SIZE<method>;
  read(stream_, {cipher, clen}, yield);
  decryptor_.decrypt({cipher, clen}, block);
}

template <CryptoMethod method, typename Stream>
size_t SSAeadAdapter<method, Stream>::recvFrame(MutableBuffer provided, Yield yield)
{
  auto lb = array<uint8_t, 2>{};
  recvBlock(lb, yield);

  auto len = ntoh<uint16_t>(lb);
  assertTrue(len <= MAX_FRAME_SIZE, PichiError::BAD_PROTO);

  auto frame = prepare(len, provided);
  recvBlock(frame, yield);

  return len;
}

template <CryptoMethod method, typename Stream>
size_t SSAeadAdapter<method, Stream>::copyTo(MutableBuffer dst)
{
  auto copied = asio::buffer_copy(asio::buffer(dst), cache_.data(), dst.size());
  cache_.consume(copied);
  return copied;
}

template <CryptoMethod method, typename Stream>
size_t SSAeadAdapter<method, Stream>::encrypt(ConstBuffer plain, MutableBuffer cipher)
{
  assertTrue(plain.size() <= MAX_FRAME_SIZE, PichiError::BAD_PROTO);
  assertTrue(cipher.size() >= plain.size() + 2 + 2 * TAG_SIZE<method>, PichiError::BAD_PROTO);

  auto lb = array<uint8_t, 2>{};
  hton(static_cast<uint16_t>(plain.size()), lb);

  auto len = encryptor_.encrypt(lb, cipher);
  len += encryptor_.encrypt(plain, cipher + len);

  return len;
}

template class SSAeadAdapter<CryptoMethod::AES_128_GCM, tcp::socket>;
template class SSAeadAdapter<CryptoMethod::AES_192_GCM, tcp::socket>;
template class SSAeadAdapter<CryptoMethod::AES_256_GCM, tcp::socket>;
template class SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305, tcp::socket>;
template class SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305, tcp::socket>;

#ifdef BUILD_TEST
template class SSAeadAdapter<CryptoMethod::AES_128_GCM, stream::TestStream>;
template class SSAeadAdapter<CryptoMethod::AES_192_GCM, stream::TestStream>;
template class SSAeadAdapter<CryptoMethod::AES_256_GCM, stream::TestStream>;
template class SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305, stream::TestStream>;
template class SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305, stream::TestStream>;
#endif  // BUILD_TEST

}  // namespace pichi::net
