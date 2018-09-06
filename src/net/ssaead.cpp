#include <array>
#include <pichi/asserts.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/ssaead.hpp>
#include <utility>

using namespace std;
using namespace pichi::crypto;
namespace asio = boost::asio;

namespace pichi::net {

template <CryptoMethod method>
SSAeadAdapter<method>::SSAeadAdapter(Socket&& socket, ConstBuffer<uint8_t> psk)
  : socket_{move(socket)}, encryptor_{psk}, decryptor_{psk}
{
}

template <CryptoMethod method> void SSAeadAdapter<method>::close() { socket_.close(); }

template <CryptoMethod method> bool SSAeadAdapter<method>::readable() const
{
  return socket_.is_open();
}

template <CryptoMethod method> bool SSAeadAdapter<method>::writable() const
{
  return socket_.is_open();
}

template <CryptoMethod method>
size_t SSAeadAdapter<method>::recv(MutableBuffer<uint8_t> plain, Yield yield)
{
  if (!ivReceived_) {
    auto iv = array<uint8_t, CryptoLength<method>::IV>{};
    read(socket_, iv, yield);
    decryptor_.setIv(iv);
    ivReceived_ = true;
  }

  if (cache_.size() > 0) return copyTo(plain);

  auto len = recvFrame(plain, yield);

  return cache_.size() == 0 ? len : copyTo(plain);
}

template <CryptoMethod method>
void SSAeadAdapter<method>::send(ConstBuffer<uint8_t> plain, Yield yield)
{
  if (!ivSent_) {
    write(socket_, encryptor_.getIv(), yield);
    ivSent_ = true;
  }

  using CipherBuffer = array<uint8_t, 2 + MAX_FRAME_SIZE + 2 * CryptoLength<method>::TAG>;

  auto cipher = CipherBuffer{};
  auto len = encrypt(plain, cipher);

  write(socket_, {cipher, len}, yield);
}

template <CryptoMethod method>
void SSAeadAdapter<method>::connect(Endpoint const& remote, Endpoint const& server, Yield yield)
{
  pichi::net::connect(server, socket_, yield);

  auto plain = array<uint8_t, 512>{};
  auto plen = serializeEndpoint(remote, plain);

  send({plain, plen}, yield);
}

template <CryptoMethod method> Endpoint SSAeadAdapter<method>::readRemote(Yield yield)
{
  return parseEndpoint([this, yield](auto dst) {
    size_t r = 0;
    while (r < dst.size()) r += recv({dst.data() + r, dst.size() - r}, yield);
  });
}

template <CryptoMethod method> void SSAeadAdapter<method>::confirm(Yield) {}

template <CryptoMethod method>
MutableBuffer<uint8_t> SSAeadAdapter<method>::prepare(size_t n, MutableBuffer<uint8_t> provided)
{
  if (n <= provided.size()) return {provided, n};
  auto buf = cache_.prepare(n);
  cache_.commit(n);
  return {buf};
}

template <CryptoMethod method>
void SSAeadAdapter<method>::recvBlock(MutableBuffer<uint8_t> block, Yield yield)
{
  using CipherBuffer = array<uint8_t, MAX_FRAME_SIZE + CryptoLength<method>::TAG>;

  auto cipher = CipherBuffer{};
  auto clen = block.size() + CryptoLength<method>::TAG;
  read(socket_, {cipher, clen}, yield);
  decryptor_.decrypt({cipher, clen}, block);
}

template <CryptoMethod method>
size_t SSAeadAdapter<method>::recvFrame(MutableBuffer<uint8_t> provided, Yield yield)
{
  auto lb = array<uint8_t, 2>{};
  recvBlock(lb, yield);

  auto len = ntoh<uint16_t>(lb);
  assertTrue(len <= MAX_FRAME_SIZE, PichiError::BAD_PROTO);

  auto frame = prepare(len, provided);
  recvBlock(frame, yield);

  return len;
}

template <CryptoMethod method> size_t SSAeadAdapter<method>::copyTo(MutableBuffer<uint8_t> dst)
{
  auto copied = asio::buffer_copy(asio::buffer(dst), cache_.data(), dst.size());
  cache_.consume(copied);
  return copied;
}

template <CryptoMethod method>
size_t SSAeadAdapter<method>::encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher)
{
  assertTrue(plain.size() <= MAX_FRAME_SIZE, PichiError::BAD_PROTO);
  assertTrue(cipher.size() >= plain.size() + 2 + 2 * CryptoLength<method>::TAG,
             PichiError::BAD_PROTO);

  auto lb = array<uint8_t, 2>{};
  hton(static_cast<uint16_t>(plain.size()), lb);

  auto len = encryptor_.encrypt(lb, cipher);
  len += encryptor_.encrypt(plain, {cipher.data() + len, cipher.size() - len});

  return len;
}

template class SSAeadAdapter<CryptoMethod::AES_128_GCM>;
template class SSAeadAdapter<CryptoMethod::AES_192_GCM>;
template class SSAeadAdapter<CryptoMethod::AES_256_GCM>;
template class SSAeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305>;
template class SSAeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305>;

} // namespace pichi::net
