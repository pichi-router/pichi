#include <array>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/ssstream.hpp>
#include <utility>

using namespace std;
using namespace pichi::crypto;

namespace pichi::net {

static size_t const MAX_HEADER_SIZE = 512;
template <typename T> using HeaderBuffer = array<T, MAX_HEADER_SIZE>;
template <typename T> using FrameBuffer = array<T, MAX_FRAME_SIZE>;

template <CryptoMethod method>
SSStreamAdapter<method>::SSStreamAdapter(Socket&& socket, ConstBuffer<uint8_t> psk)
  : socket_{move(socket)}, encryptor_{psk}, decryptor_{psk}
{
}

template <CryptoMethod method>
size_t SSStreamAdapter<method>::recv(MutableBuffer<uint8_t> plain, Yield yield)
{
  if (!ivReceived_) {
    auto iv = array<uint8_t, IV_SIZE<method>>{};
    read(socket_, iv, yield);
    decryptor_.setIv(iv);
    ivReceived_ = true;
  }

  auto cipher = FrameBuffer<uint8_t>{};
  auto len = socket_.async_read_some(boost::asio::buffer(cipher, plain.size()), yield);
  return decryptor_.decrypt({cipher, len}, plain);
}

template <CryptoMethod method>
void SSStreamAdapter<method>::send(ConstBuffer<uint8_t> plain, Yield yield)
{
  if (!ivSent_) {
    write(socket_, encryptor_.getIv(), yield);
    ivSent_ = true;
  }

  auto data = plain.data();
  auto size = plain.size();
  auto cipher = FrameBuffer<uint8_t>{};
  while (size > 0) {
    auto consumed = size > cipher.size() ? cipher.size() : size;
    auto len = encryptor_.encrypt({data, consumed}, {cipher, consumed});
    write(socket_, {cipher, len}, yield);
    size -= consumed;
    data += consumed;
  }
}

template <CryptoMethod method> void SSStreamAdapter<method>::close() { socket_.close(); }

template <CryptoMethod method> bool SSStreamAdapter<method>::readable() const
{
  return socket_.is_open();
}

template <CryptoMethod method> bool SSStreamAdapter<method>::writable() const
{
  return socket_.is_open();
}

template <CryptoMethod method> Endpoint SSStreamAdapter<method>::readRemote(Yield yield)
{
  return parseEndpoint([this, yield](auto dst) { recv(dst, yield); });
}

template <CryptoMethod method> void SSStreamAdapter<method>::confirm(Yield) {}

template <CryptoMethod method>
void SSStreamAdapter<method>::connect(Endpoint const& remote, Endpoint const& server, Yield yield)
{
  pichi::net::connect(server, socket_, yield);

  auto plain = HeaderBuffer<uint8_t>{};
  auto plen = serializeEndpoint(remote, plain);

  send({plain, plen}, yield);
}

template class SSStreamAdapter<CryptoMethod::RC4_MD5>;
template class SSStreamAdapter<CryptoMethod::BF_CFB>;
template class SSStreamAdapter<CryptoMethod::AES_128_CTR>;
template class SSStreamAdapter<CryptoMethod::AES_192_CTR>;
template class SSStreamAdapter<CryptoMethod::AES_256_CTR>;
template class SSStreamAdapter<CryptoMethod::AES_128_CFB>;
template class SSStreamAdapter<CryptoMethod::AES_192_CFB>;
template class SSStreamAdapter<CryptoMethod::AES_256_CFB>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_128_CFB>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_192_CFB>;
template class SSStreamAdapter<CryptoMethod::CAMELLIA_256_CFB>;
template class SSStreamAdapter<CryptoMethod::CHACHA20>;
template class SSStreamAdapter<CryptoMethod::SALSA20>;
template class SSStreamAdapter<CryptoMethod::CHACHA20_IETF>;

} // namespace pichi::net
