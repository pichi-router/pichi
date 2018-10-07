#include <array>
#include <iostream>
#include <pichi/asserts.hpp>
#include <pichi/crypto/hash.hpp>
#include <pichi/crypto/stream.hpp>
#include <sodium.h>

using namespace std;

namespace pichi::crypto {

template <CryptoMethod method> static size_t ctrBlockSize()
{
  return helpers::isAesCtr<method>() ? 16 : 0;
}

template <typename SodiumFunc>
static size_t sodiumHelper(SodiumFunc&& func, ConstBuffer<uint8_t> key, size_t offset,
                           ConstBuffer<uint8_t> input, MutableBuffer<uint8_t> iv,
                           MutableBuffer<uint8_t> output)
{
  auto padding = offset % 64;
  auto ib = vector<uint8_t>(input.size() + padding, 0);
  auto ob = vector<uint8_t>(input.size() + padding, 0);

  copy(input.begin(), input.end(), ib.begin() + padding);
  auto ret = func(ob.data(), ib.data(), ib.size(), iv.data(), offset / 64, key.data());
  assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  copy(ob.begin() + padding, ob.end(), output.begin());

  return offset + input.size();
}

template <CryptoMethod method>
static void initialize(StreamContext<method>& ctx, ConstBuffer<uint8_t> key,
                       ConstBuffer<uint8_t> iv)
{
  assertTrue(key.size() == KEY_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertTrue(iv.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  if constexpr (helpers::isArc<method>()) {
    auto md5 = Hash<HashAlgorithm::MD5>{};
    md5.append(key);
    md5.append(iv);
    auto skey = array<uint8_t, HashTraits<HashAlgorithm::MD5>::length>{};
    md5.hash(skey);

    mbedtls_arc4_init(&ctx);
    mbedtls_arc4_setup(&ctx, skey.data(), skey.size());
  }
  else if constexpr (helpers::isBlowfish<method>()) {
    mbedtls_blowfish_init(&ctx);
    assertTrue(mbedtls_blowfish_setkey(&ctx, key.data(), key.size() * 8) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isAesCfb<method>() || helpers::isAesCtr<method>()) {
    mbedtls_aes_init(&ctx);
    assertTrue(mbedtls_aes_setkey_enc(&ctx, key.data(), key.size() * 8) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isCamellia<method>()) {
    mbedtls_camellia_init(&ctx);
    assertTrue(mbedtls_camellia_setkey_enc(&ctx, key.data(), key.size() * 8) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isSodiumStream<method>()) {
    ctx.assign(begin(key), end(key));
  }
  else
    static_assert(helpers::DependentFalse<method>::value);
}

template <CryptoMethod method> static void release(StreamContext<method>& ctx)
{
  if constexpr (helpers::isArc<method>())
    mbedtls_arc4_free(&ctx);
  else if constexpr (helpers::isBlowfish<method>())
    mbedtls_blowfish_free(&ctx);
  else if constexpr (helpers::isAesCtr<method>() || helpers::isAesCfb<method>())
    mbedtls_aes_free(&ctx);
  else if constexpr (helpers::isCamellia<method>())
    mbedtls_camellia_free(&ctx);
  else if constexpr (helpers::isSodiumStream<method>()) {
    // ignore
  }
  else
    static_assert(helpers::DependentFalse<method>::value);
}

template <CryptoMethod method>
static size_t encrypt(StreamContext<method>& ctx, size_t offset, ConstBuffer<uint8_t> plain,
                      MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> ctrBlock,
                      MutableBuffer<uint8_t> cipher)
{
  assertTrue(plain.size() <= cipher.size(), PichiError::CRYPTO_ERROR);
  assertTrue(iv.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertTrue(ctrBlock.size() == ctrBlockSize<method>(), PichiError::CRYPTO_ERROR);
  if constexpr (helpers::isArc<method>()) {
    assertTrue(mbedtls_arc4_crypt(&ctx, plain.size(), plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
    offset += plain.size();
  }
  else if constexpr (helpers::isBlowfish<method>()) {
    assertTrue(mbedtls_blowfish_crypt_cfb64(&ctx, MBEDTLS_BLOWFISH_ENCRYPT, plain.size(), &offset,
                                            iv.data(), plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isAesCtr<method>()) {
    assertTrue(mbedtls_aes_crypt_ctr(&ctx, plain.size(), &offset, iv.data(), ctrBlock.data(),
                                     plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isAesCfb<method>()) {
    assertTrue(mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_ENCRYPT, plain.size(), &offset, iv.data(),
                                        plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isCamellia<method>()) {
    assertTrue(mbedtls_camellia_crypt_cfb128(&ctx, MBEDTLS_CAMELLIA_ENCRYPT, plain.size(), &offset,
                                             iv.data(), plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (method == CryptoMethod::CHACHA20) {
    offset = sodiumHelper(crypto_stream_chacha20_xor_ic, ctx, offset, plain, iv, cipher);
  }
  else if constexpr (method == CryptoMethod::SALSA20) {
    offset = sodiumHelper(crypto_stream_salsa20_xor_ic, ctx, offset, plain, iv, cipher);
  }
  else if constexpr (method == CryptoMethod::CHACHA20_IETF) {
    offset = sodiumHelper(crypto_stream_chacha20_ietf_xor_ic, ctx, offset, plain, iv, cipher);
  }
  else
    static_assert(helpers::DependentFalse<method>::value);
  return offset;
}

template <CryptoMethod method>
static size_t decrypt(StreamContext<method>& ctx, size_t offset, ConstBuffer<uint8_t> cipher,
                      MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> ctrBlock,
                      MutableBuffer<uint8_t> plain)
{
  assertTrue(plain.size() >= cipher.size(), PichiError::CRYPTO_ERROR);
  assertTrue(iv.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertTrue(ctrBlock.size() == ctrBlockSize<method>(), PichiError::CRYPTO_ERROR);
  if constexpr (helpers::isArc<method>()) {
    assertTrue(mbedtls_arc4_crypt(&ctx, cipher.size(), cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
    offset += cipher.size();
  }
  else if constexpr (helpers::isBlowfish<method>()) {
    assertTrue(mbedtls_blowfish_crypt_cfb64(&ctx, MBEDTLS_BLOWFISH_DECRYPT, cipher.size(), &offset,
                                            iv.data(), cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isAesCtr<method>()) {
    assertTrue(mbedtls_aes_crypt_ctr(&ctx, cipher.size(), &offset, iv.data(), ctrBlock.data(),
                                     cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isAesCfb<method>()) {
    assertTrue(mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_DECRYPT, cipher.size(), &offset,
                                        iv.data(), cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (helpers::isCamellia<method>()) {
    assertTrue(mbedtls_camellia_crypt_cfb128(&ctx, MBEDTLS_CAMELLIA_DECRYPT, cipher.size(), &offset,
                                             iv.data(), cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (method == CryptoMethod::CHACHA20) {
    offset = sodiumHelper(crypto_stream_chacha20_xor_ic, ctx, offset, cipher, iv, plain);
  }
  else if constexpr (method == CryptoMethod::SALSA20) {
    offset = sodiumHelper(crypto_stream_salsa20_xor_ic, ctx, offset, cipher, iv, plain);
  }
  else if constexpr (method == CryptoMethod::CHACHA20_IETF) {
    offset = sodiumHelper(crypto_stream_chacha20_ietf_xor_ic, ctx, offset, cipher, iv, plain);
  }
  else
    static_assert(helpers::DependentFalse<method>::value);
  return offset;
}

template <CryptoMethod method>
StreamEncryptor<method>::StreamEncryptor(ConstBuffer<uint8_t> key, ConstBuffer<uint8_t> iv)
  : iv_{iv.begin(), iv.end()}, ctrBlock_(ctrBlockSize<method>(), 0)
{
  if (iv_.empty()) {
    iv_.resize(IV_SIZE<method>);
    randombytes_buf(iv_.data(), iv_.size());
  }
  assertTrue(iv_.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  initialize<method>(ctx_, key, iv_);
}

template <CryptoMethod method> StreamEncryptor<method>::~StreamEncryptor()
{
  release<method>(ctx_);
}

template <CryptoMethod method> ConstBuffer<uint8_t> StreamEncryptor<method>::getIv() const
{
  // FIXME iv might be chaged after every invocation of this::encrypt
  return {iv_};
}

template <CryptoMethod method>
size_t StreamEncryptor<method>::encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher)
{
  assertTrue(cipher.size() >= plain.size(), PichiError::CRYPTO_ERROR);
  offset_ = pichi::crypto::encrypt<method>(ctx_, offset_, plain, iv_, ctrBlock_, cipher);
  return plain.size();
}

template class StreamEncryptor<CryptoMethod::RC4_MD5>;
template class StreamEncryptor<CryptoMethod::BF_CFB>;
template class StreamEncryptor<CryptoMethod::AES_128_CTR>;
template class StreamEncryptor<CryptoMethod::AES_192_CTR>;
template class StreamEncryptor<CryptoMethod::AES_256_CTR>;
template class StreamEncryptor<CryptoMethod::AES_128_CFB>;
template class StreamEncryptor<CryptoMethod::AES_192_CFB>;
template class StreamEncryptor<CryptoMethod::AES_256_CFB>;
template class StreamEncryptor<CryptoMethod::CAMELLIA_128_CFB>;
template class StreamEncryptor<CryptoMethod::CAMELLIA_192_CFB>;
template class StreamEncryptor<CryptoMethod::CAMELLIA_256_CFB>;
template class StreamEncryptor<CryptoMethod::CHACHA20>;
template class StreamEncryptor<CryptoMethod::SALSA20>;
template class StreamEncryptor<CryptoMethod::CHACHA20_IETF>;

template <CryptoMethod method>
StreamDecryptor<method>::StreamDecryptor(ConstBuffer<uint8_t> key)
  : iv_{begin(key), end(key)}, ctrBlock_(ctrBlockSize<method>(), 0)
{
}

template <CryptoMethod method> StreamDecryptor<method>::~StreamDecryptor()
{
  if (initialized_) release<method>(ctx_);
}

template <CryptoMethod method> size_t StreamDecryptor<method>::getIvSize() const
{
  return IV_SIZE<method>;
}

template <CryptoMethod method> void StreamDecryptor<method>::setIv(ConstBuffer<uint8_t> iv)
{
  auto key = move(iv_);
  assertFalse(initialized_, PichiError::MISC);
  assertTrue(iv_.empty(), PichiError::CRYPTO_ERROR);
  assertTrue(iv.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  iv_.assign(begin(iv), end(iv));
  initialize<method>(ctx_, key, iv_);
  initialized_ = true;
}

template <CryptoMethod method>
size_t StreamDecryptor<method>::decrypt(ConstBuffer<uint8_t> cipher, MutableBuffer<uint8_t> plain)
{
  assertTrue(iv_.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  offset_ = pichi::crypto::decrypt<method>(ctx_, offset_, cipher, iv_, ctrBlock_, plain);
  return cipher.size();
}

template class StreamDecryptor<CryptoMethod::RC4_MD5>;
template class StreamDecryptor<CryptoMethod::BF_CFB>;
template class StreamDecryptor<CryptoMethod::AES_128_CTR>;
template class StreamDecryptor<CryptoMethod::AES_192_CTR>;
template class StreamDecryptor<CryptoMethod::AES_256_CTR>;
template class StreamDecryptor<CryptoMethod::AES_128_CFB>;
template class StreamDecryptor<CryptoMethod::AES_192_CFB>;
template class StreamDecryptor<CryptoMethod::AES_256_CFB>;
template class StreamDecryptor<CryptoMethod::CAMELLIA_128_CFB>;
template class StreamDecryptor<CryptoMethod::CAMELLIA_192_CFB>;
template class StreamDecryptor<CryptoMethod::CAMELLIA_256_CFB>;
template class StreamDecryptor<CryptoMethod::CHACHA20>;
template class StreamDecryptor<CryptoMethod::SALSA20>;
template class StreamDecryptor<CryptoMethod::CHACHA20_IETF>;

} // namespace pichi::crypto
