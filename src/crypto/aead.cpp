#include <pichi/asserts.hpp>
#include <pichi/crypto/aead.hpp>
#include <pichi/crypto/hash.hpp>
#include <sodium.h>

using namespace std;

namespace pichi {
namespace crypto {

// TODO okmLen might be replaced with template argument.
struct Initializations {
  static void gcm(mbedtls_gcm_context& ctx, ConstBuffer<uint8_t> ikm, ConstBuffer<uint8_t> salt,
                  size_t okmLen)
  {
    auto skey = vector<uint8_t>(okmLen, 0);
    hkdf<HashAlgorithm::SHA1>(skey, ikm, salt);

    mbedtls_gcm_init(&ctx);
    auto ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, skey.data(), skey.size() * 8);
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void sodium(vector<uint8_t>& ctx, ConstBuffer<uint8_t> ikm, ConstBuffer<uint8_t> salt,
                     size_t okmLen)
  {
    ctx.resize(okmLen);
    hkdf<HashAlgorithm::SHA1>(ctx, ikm, salt);
  }
};

struct Releasings {
  static void gcm(mbedtls_gcm_context& ctx) { mbedtls_gcm_free(&ctx); }
  static void sodium(vector<uint8_t>& ctx) {}
};

struct Encryptions {
  static void gcm(mbedtls_gcm_context& ctx, ConstBuffer<uint8_t> nonce, ConstBuffer<uint8_t> input,
                  MutableBuffer<uint8_t> output, size_t tlen)
  {
    auto ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, input.size(), nonce.data(),
                                         nonce.size(), nullptr, 0, input.data(), output.data(),
                                         tlen, output.data() + input.size());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void chacha20(vector<uint8_t>& ctx, ConstBuffer<uint8_t> nonce, ConstBuffer<uint8_t> input,
                       MutableBuffer<uint8_t> output, size_t tlen)
  {
    unsigned long long clen = input.size() + tlen;
    auto ret =
        crypto_aead_chacha20poly1305_ietf_encrypt(output.data(), &clen, input.data(), input.size(),
                                                  nullptr, 0, nullptr, nonce.data(), ctx.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void xchacha20(vector<uint8_t>& ctx, ConstBuffer<uint8_t> nonce,
                        ConstBuffer<uint8_t> input, MutableBuffer<uint8_t> output, size_t tlen)
  {
    unsigned long long clen = input.size() + tlen;
    auto ret =
        crypto_aead_xchacha20poly1305_ietf_encrypt(output.data(), &clen, input.data(), input.size(),
                                                   nullptr, 0, nullptr, nonce.data(), ctx.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }
};

struct Decryptions {
  static void gcm(mbedtls_gcm_context& ctx, ConstBuffer<uint8_t> nonce, ConstBuffer<uint8_t> input,
                  MutableBuffer<uint8_t> output, size_t tlen)
  {
    auto ret = mbedtls_gcm_auth_decrypt(&ctx, input.size() - tlen, nonce.data(), nonce.size(),
                                        nullptr, 0, input.data() + input.size() - tlen, tlen,
                                        input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void chacha20(vector<uint8_t>& ctx, ConstBuffer<uint8_t> nonce, ConstBuffer<uint8_t> input,
                       MutableBuffer<uint8_t> output, size_t tlen)
  {
    auto mlen = 0ull;
    auto ret = crypto_aead_chacha20poly1305_ietf_decrypt(output.data(), &mlen, nullptr,
                                                         input.data(), input.size(), nullptr, 0,
                                                         nonce.data(), ctx.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void xchacha20(vector<uint8_t>& ctx, ConstBuffer<uint8_t> nonce,
                        ConstBuffer<uint8_t> input, MutableBuffer<uint8_t> output, size_t tlen)
  {
    auto mlen = 0ull;
    auto ret = crypto_aead_xchacha20poly1305_ietf_decrypt(output.data(), &mlen, nullptr,
                                                          input.data(), input.size(), nullptr, 0,
                                                          nonce.data(), ctx.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }
};

template <CryptoMethod method> struct AeadTrait {
};

template <> struct AeadTrait<CryptoMethod::AES_128_GCM> {
  static constexpr auto initialize = Initializations::gcm;
  static constexpr auto release = Releasings::gcm;
  static constexpr auto encrypt = Encryptions::gcm;
  static constexpr auto decrypt = Decryptions::gcm;
};

template <> struct AeadTrait<CryptoMethod::AES_192_GCM> {
  static constexpr auto initialize = Initializations::gcm;
  static constexpr auto release = Releasings::gcm;
  static constexpr auto encrypt = Encryptions::gcm;
  static constexpr auto decrypt = Decryptions::gcm;
};

template <> struct AeadTrait<CryptoMethod::AES_256_GCM> {
  static constexpr auto initialize = Initializations::gcm;
  static constexpr auto release = Releasings::gcm;
  static constexpr auto encrypt = Encryptions::gcm;
  static constexpr auto decrypt = Decryptions::gcm;
};

template <> struct AeadTrait<CryptoMethod::CHACHA20_IETF_POLY1305> {
  static constexpr auto initialize = Initializations::sodium;
  static constexpr auto release = Releasings::sodium;
  static constexpr auto encrypt = Encryptions::chacha20;
  static constexpr auto decrypt = Decryptions::chacha20;
};

template <> struct AeadTrait<CryptoMethod::XCHACHA20_IETF_POLY1305> {
  static constexpr auto initialize = Initializations::sodium;
  static constexpr auto release = Releasings::sodium;
  static constexpr auto encrypt = Encryptions::xchacha20;
  static constexpr auto decrypt = Decryptions::xchacha20;
};

template <CryptoMethod method>
AeadEncryptor<method>::AeadEncryptor(ConstBuffer<uint8_t> key, ConstBuffer<uint8_t> salt)
  : nonce_(CryptoLength<method>::NONCE, 0), salt_{salt.begin(), salt.end()}
{
  if (salt_.empty()) {
    salt_.resize(CryptoLength<method>::IV);
    randombytes_buf(salt_.data(), salt_.size());
  }
  assertTrue(salt_.size() == CryptoLength<method>::IV, PichiError::CRYPTO_ERROR);
  AeadTrait<method>::initialize(ctx_, key, salt_, CryptoLength<method>::KEY);
}

template <CryptoMethod method> AeadEncryptor<method>::~AeadEncryptor()
{
  AeadTrait<method>::release(ctx_);
}

template <CryptoMethod method> ConstBuffer<uint8_t> AeadEncryptor<method>::getIv() const
{
  return salt_;
}

template <CryptoMethod method>
size_t AeadEncryptor<method>::encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher)
{
  assertTrue(plain.size() <= 0x3fff, PichiError::CRYPTO_ERROR);
  assertTrue(cipher.size() >= plain.size() + CryptoLength<method>::TAG, PichiError::CRYPTO_ERROR);
  AeadTrait<method>::encrypt(ctx_, nonce_, plain, cipher, CryptoLength<method>::TAG);
  sodium_increment(nonce_.data(), nonce_.size());
  return plain.size() + CryptoLength<method>::TAG;
}

template class AeadEncryptor<CryptoMethod::AES_128_GCM>;
template class AeadEncryptor<CryptoMethod::AES_192_GCM>;
template class AeadEncryptor<CryptoMethod::AES_256_GCM>;
template class AeadEncryptor<CryptoMethod::CHACHA20_IETF_POLY1305>;
template class AeadEncryptor<CryptoMethod::XCHACHA20_IETF_POLY1305>;

template <CryptoMethod method>
AeadDecryptor<method>::AeadDecryptor(ConstBuffer<uint8_t> key)
  : okm_{key.begin(), key.end()}, nonce_(CryptoLength<method>::NONCE, 0)
{
}

template <CryptoMethod method> AeadDecryptor<method>::~AeadDecryptor()
{
  AeadTrait<method>::release(ctx_);
}

template <CryptoMethod method> size_t AeadDecryptor<method>::getIvSize() const
{
  return CryptoLength<method>::IV;
}

template <CryptoMethod method> void AeadDecryptor<method>::setIv(ConstBuffer<uint8_t> iv)
{
  assertTrue(iv.size() == CryptoLength<method>::IV, PichiError::CRYPTO_ERROR);
  assertFalse(okm_.empty(), PichiError::CRYPTO_ERROR);
  AeadTrait<method>::initialize(ctx_, okm_, iv, CryptoLength<method>::KEY);
  okm_.clear();
}

template <CryptoMethod method>
size_t AeadDecryptor<method>::decrypt(ConstBuffer<uint8_t> cipher, MutableBuffer<uint8_t> plain)
{
  assertTrue(okm_.empty(), PichiError::CRYPTO_ERROR);
  assertTrue(cipher.size() > CryptoLength<method>::TAG, PichiError::CRYPTO_ERROR);
  assertTrue(plain.size() >= cipher.size() - CryptoLength<method>::TAG, PichiError::CRYPTO_ERROR);
  AeadTrait<method>::decrypt(ctx_, nonce_, cipher, plain, CryptoLength<method>::TAG);
  sodium_increment(nonce_.data(), nonce_.size());
  return cipher.size() - CryptoLength<method>::TAG;
}

template class AeadDecryptor<CryptoMethod::AES_128_GCM>;
template class AeadDecryptor<CryptoMethod::AES_192_GCM>;
template class AeadDecryptor<CryptoMethod::AES_256_GCM>;
template class AeadDecryptor<CryptoMethod::CHACHA20_IETF_POLY1305>;
template class AeadDecryptor<CryptoMethod::XCHACHA20_IETF_POLY1305>;

} // namespace crypto
} // namespace pichi
