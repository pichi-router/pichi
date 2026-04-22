#include <botan/hash.h>
#include <botan/kdf.h>
#include <botan/sodium.h>
#include <pichi/common/literals.hpp>
#include <pichi/stream/shadowsocks.hpp>
#include <string_view>
#include <unordered_map>

using namespace std::literals;
namespace asio  = boost::asio;
namespace rngs  = std::ranges;
namespace views = rngs::views;

namespace pichi::stream::detail {

static auto const ALGO_MAP = std::unordered_map<CryptoMethod, std::string_view>{
    {            CryptoMethod::AES_128_GCM,      "AES-128/GCM"sv},
    {            CryptoMethod::AES_192_GCM,      "AES-192/GCM"sv},
    {            CryptoMethod::AES_256_GCM,      "AES-256/GCM"sv},
    { CryptoMethod::CHACHA20_IETF_POLY1305, "ChaCha20Poly1305"sv},
    {CryptoMethod::XCHACHA20_IETF_POLY1305, "ChaCha20Poly1305"sv},
};

static auto const NONCE_SIZE = std::unordered_map<CryptoMethod, size_t>{
    {            CryptoMethod::AES_128_GCM, 12_sz},
    {            CryptoMethod::AES_192_GCM, 12_sz},
    {            CryptoMethod::AES_256_GCM, 12_sz},
    { CryptoMethod::CHACHA20_IETF_POLY1305, 12_sz},
    {CryptoMethod::XCHACHA20_IETF_POLY1305, 24_sz},
};

static auto const KEY_SIZE = std::unordered_map<CryptoMethod, size_t>{
    {            CryptoMethod::AES_128_GCM, 16_sz},
    {            CryptoMethod::AES_192_GCM, 24_sz},
    {            CryptoMethod::AES_256_GCM, 32_sz},
    { CryptoMethod::CHACHA20_IETF_POLY1305, 32_sz},
    {CryptoMethod::XCHACHA20_IETF_POLY1305, 32_sz},
};

static auto random_salt(CryptoMethod method)
{
  auto len  = KEY_SIZE.at(method);
  auto salt = std::vector<uint8_t>(len, 0_u8);
  Botan::Sodium::randombytes_buf(rngs::data(salt), len);
  return salt;
}

static auto generate_psk(ConstBuffer pw, MutableBuffer data, size_t size)
{
  auto psk = MutableBuffer{data, size};

  auto tmp = ConstBuffer{};
  for (auto i = 0_sz; i < size; i += rngs::size(tmp)) {
    auto md5 = Botan::HashFunction::create_or_throw("MD5");
    auto len = md5->output_length();

    md5->update(tmp);
    md5->update(pw);
    md5->final(data);

    tmp = {data, len};
    data += len;
  }

  return psk;
}

Cryptor::Cryptor(CryptoMethod method, Botan::Cipher_Dir direction)
  : cryptor_{Botan::Cipher_Mode::create_or_throw(ALGO_MAP.at(method), direction)},
    nonce_(NONCE_SIZE.at(method), 0_u8)
{
}

void Cryptor::set_psk(ConstBuffer pw, ConstBuffer salt)
{
  auto data = std::array<uint8_t, 1024>{};
  auto psk  = generate_psk(pw, data, rngs::size(salt));
  cryptor_->set_key(Botan::KDF::create_or_throw("HKDF(SHA-1)")
                        ->derive_key(rngs::size(salt), psk, salt, ConstBuffer{"ss-subkey"sv}));
}

size_t Cryptor::process(ConstBuffer orig, MutableBuffer dest)
{
  auto n = rngs::size(orig) - cryptor_->minimum_final_size();
  rngs::copy(orig | views::take(n), rngs::begin(dest));
  cryptor_->start(nonce_);
  cryptor_->process(dest | views::take(n));

  auto v = orig | views::drop(n);
  auto t = std::vector<uint8_t>{rngs::begin(v), rngs::end(v)};
  cryptor_->finish(t);
  rngs::copy(t, rngs::begin(dest | views::drop(n)));

  Botan::Sodium::sodium_increment(rngs::data(nonce_), rngs::size(nonce_));
  return n + rngs::size(t);
}

Encryptor::Encryptor(CryptoMethod method, ConstBuffer pw)
  : cryptor_{method, Botan::Cipher_Dir::Encryption}, salt_{random_salt(method)}
{
  cryptor_.set_psk(pw, salt_);
}

ConstBuffer Encryptor::salt() const { return salt_; }

size_t Encryptor::process(ConstBuffer plain, MutableBuffer cipher)
{
  salt_.clear();
  return cryptor_.process(plain, cipher);
}

Decryptor::Decryptor(CryptoMethod method)
  : cryptor_{method, Botan::Cipher_Dir::Decryption}, salt_(KEY_SIZE.at(method), 0_u8)
{
}

MutableBuffer Decryptor::salt() { return salt_; }

void Decryptor::set_psk(ConstBuffer pw)
{
  cryptor_.set_psk(pw, salt_);
  salt_.clear();
}

size_t Decryptor::process(ConstBuffer cipher, MutableBuffer plain)
{
  return cryptor_.process(cipher, plain);
}

bool Cache::empty() const { return data_.size() == 0; };

size_t Cache::copy(MutableBuffer dst)
{
  auto n = asio::buffer_copy(asio::buffer(rngs::data(dst), rngs::size(dst)), data_.cdata());
  data_.consume(n);
  return n;
}

MutableBuffer Cache::prepare(MutableBuffer provided, size_t n)
{
  if (n <= rngs::size(provided)) return {provided, n};
  auto buf = data_.prepare(n);
  data_.commit(n);
  return buf;
}

}  // namespace pichi::stream::detail
