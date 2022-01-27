#include <pichi/common/config.hpp>
// Include config.hpp first
#include <iostream>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/crypto/hash.hpp>
#include <pichi/crypto/stream.hpp>
#include <sodium/crypto_stream_chacha20.h>
#include <sodium/crypto_stream_salsa20.h>
#include <sodium/randombytes.h>

using namespace std;

namespace pichi::crypto {

template <typename SodiumFunc>
static size_t sodiumHelper(SodiumFunc&& func, ConstBuffer<uint8_t> key, size_t offset,
                           ConstBuffer<uint8_t> input, MutableBuffer<uint8_t> iv,
                           MutableBuffer<uint8_t> output)
{
  assertTrue(output.size() >= input.size(), PichiError::MISC);

  using IC =
      conditional_t<is_same_v<decay_t<SodiumFunc>, decltype(&crypto_stream_chacha20_ietf_xor_ic)>,
                    uint32_t, uint64_t>;

  auto ic = static_cast<IC>(offset / 64);
  auto padding = offset % 64;
  auto converted = min((64 - padding) % 64, input.size());

  if (padding != 0) {
    auto ib = array<uint8_t, 64>{};
    auto ob = array<uint8_t, 64>{};
    copy_n(cbegin(input), converted, begin(ib) + padding);
    assertTrue(func(ob.data(), ib.data(), padding + converted, iv.data(), ic, key.data()) == 0,
               PichiError::CRYPTO_ERROR);
    copy_n(cbegin(ob) + padding, converted, begin(output));
  }
  if (converted < input.size()) {
    if (converted > 0) ++ic;
    assertTrue(func(output.data() + converted, input.data() + converted, input.size() - converted,
                    iv.data(), ic, key.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }

  return offset + input.size();
}

template <CryptoMethod method>
static void initialize(StreamContext<method>& ctx, ConstBuffer<uint8_t> key,
                       ConstBuffer<uint8_t> iv)
{
  suppressC4100(ctx);
  assertTrue(key.size() == KEY_SIZE<method>, PichiError::CRYPTO_ERROR);
  assertTrue(iv.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  if constexpr (detail::isArc<method>()) {
    auto md5 = Hash<HashAlgorithm::MD5>{};
    md5.append(key);
    md5.append(iv);
    auto skey = array<uint8_t, HashTraits<HashAlgorithm::MD5>::length>{};
    md5.hash(skey);

    mbedtls_arc4_init(&ctx);
    mbedtls_arc4_setup(&ctx, skey.data(), static_cast<unsigned int>(skey.size()));
  }
  else if constexpr (detail::isBlowfish<method>()) {
    mbedtls_blowfish_init(&ctx);
    assertTrue(
        mbedtls_blowfish_setkey(&ctx, key.data(), static_cast<unsigned int>(key.size() * 8)) == 0,
        PichiError::CRYPTO_ERROR);
  }
  else if constexpr (detail::isAesCfb<method>() || detail::isAesCtr<method>()) {
    mbedtls_aes_init(&ctx);
    assertTrue(
        mbedtls_aes_setkey_enc(&ctx, key.data(), static_cast<unsigned int>(key.size() * 8)) == 0,
        PichiError::CRYPTO_ERROR);
  }
  else if constexpr (detail::isCamellia<method>()) {
    mbedtls_camellia_init(&ctx);
    assertTrue(mbedtls_camellia_setkey_enc(&ctx, key.data(),
                                           static_cast<unsigned int>(key.size() * 8)) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (detail::isSodiumStream<method>()) {
    copy(cbegin(key), cend(key), begin(ctx));
  }
  else
    static_assert(detail::DependentFalse<method>::value);
}

template <CryptoMethod method> static void release(StreamContext<method>& ctx)
{
  suppressC4100(ctx);
  if constexpr (detail::isArc<method>())
    mbedtls_arc4_free(&ctx);
  else if constexpr (detail::isBlowfish<method>())
    mbedtls_blowfish_free(&ctx);
  else if constexpr (detail::isAesCtr<method>() || detail::isAesCfb<method>())
    mbedtls_aes_free(&ctx);
  else if constexpr (detail::isCamellia<method>())
    mbedtls_camellia_free(&ctx);
  else if constexpr (detail::isSodiumStream<method>()) {
    // ignore
  }
  else
    static_assert(detail::DependentFalse<method>::value);
}

template <CryptoMethod method>
static size_t encrypt(StreamContext<method>& ctx, size_t offset, ConstBuffer<uint8_t> plain,
                      MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> cipher)
{
  suppressC4100(ctx);
  assertTrue(plain.size() <= cipher.size(), PichiError::CRYPTO_ERROR);
  assertTrue(iv.size() >= IV_SIZE<method> + BLK_SIZE<method>, PichiError::CRYPTO_ERROR);
  if constexpr (detail::isAesCtr<method>()) {
    assertTrue(mbedtls_aes_crypt_ctr(&ctx, plain.size(), &offset, iv.data(),
                                     iv.data() + IV_SIZE<method>, plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (detail::isAesCfb<method>()) {
    assertTrue(mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_ENCRYPT, plain.size(), &offset, iv.data(),
                                        plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (detail::isCamellia<method>()) {
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
#if MBEDTLS_VERSION_MAJOR < 3
  else if constexpr (detail::isArc<method>()) {
    assertTrue(mbedtls_arc4_crypt(&ctx, plain.size(), plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
    offset += plain.size();
  }
  else if constexpr (detail::isBlowfish<method>()) {
    assertTrue(mbedtls_blowfish_crypt_cfb64(&ctx, MBEDTLS_BLOWFISH_ENCRYPT, plain.size(), &offset,
                                            iv.data(), plain.data(), cipher.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
#endif  // MBEDTLS_VERSION_MAJOR < 3
  else
    static_assert(detail::DependentFalse<method>::value);
  return offset;
}

template <CryptoMethod method>
static size_t decrypt(StreamContext<method>& ctx, size_t offset, ConstBuffer<uint8_t> cipher,
                      MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> plain)
{
  suppressC4100(ctx);
  assertTrue(plain.size() >= cipher.size(), PichiError::CRYPTO_ERROR);
  assertTrue(iv.size() >= IV_SIZE<method> + BLK_SIZE<method>, PichiError::CRYPTO_ERROR);
  if constexpr (detail::isAesCtr<method>()) {
    assertTrue(mbedtls_aes_crypt_ctr(&ctx, cipher.size(), &offset, iv.data(),
                                     iv.data() + IV_SIZE<method>, cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (detail::isAesCfb<method>()) {
    assertTrue(mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_DECRYPT, cipher.size(), &offset,
                                        iv.data(), cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
  else if constexpr (detail::isCamellia<method>()) {
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
#if MBEDTLS_VERSION_MAJOR < 3
  else if constexpr (detail::isArc<method>()) {
    assertTrue(mbedtls_arc4_crypt(&ctx, cipher.size(), cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
    offset += cipher.size();
  }
  else if constexpr (detail::isBlowfish<method>()) {
    assertTrue(mbedtls_blowfish_crypt_cfb64(&ctx, MBEDTLS_BLOWFISH_DECRYPT, cipher.size(), &offset,
                                            iv.data(), cipher.data(), plain.data()) == 0,
               PichiError::CRYPTO_ERROR);
  }
#endif  // MBEDTLS_VERSION_MAJOR < 3
  else
    static_assert(detail::DependentFalse<method>::value);
  return offset;
}

template <CryptoMethod method>
StreamEncryptor<method>::StreamEncryptor(ConstBuffer<uint8_t> key, ConstBuffer<uint8_t> iv)
{
  if (iv.size() == 0)
    randombytes_buf(iv_.data(), IV_SIZE<method>);
  else {
    assertTrue(iv.size() >= IV_SIZE<method>, PichiError::CRYPTO_ERROR);
    copy_n(cbegin(iv), IV_SIZE<method>, begin(iv_));
  }
  initialize<method>(ctx_, key, {iv_, IV_SIZE<method>});
}

template <CryptoMethod method> StreamEncryptor<method>::~StreamEncryptor()
{
  release<method>(ctx_);
}

template <CryptoMethod method> ConstBuffer<uint8_t> StreamEncryptor<method>::getIv() const
{
  // FIXME iv might be chaged after every invocation of this::encrypt
  return {iv_, IV_SIZE<method>};
}

template <CryptoMethod method>
size_t StreamEncryptor<method>::encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher)
{
  assertTrue(cipher.size() >= plain.size(), PichiError::CRYPTO_ERROR);
  offset_ = pichi::crypto::encrypt<method>(ctx_, offset_, plain, iv_, cipher);
  return plain.size();
}

#if MBEDTLS_VERSION_MAJOR < 3
template class StreamEncryptor<CryptoMethod::RC4_MD5>;
template class StreamEncryptor<CryptoMethod::BF_CFB>;
#endif  // MBEDTLS_VERSION_MAJOR < 3
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

template <CryptoMethod method> StreamDecryptor<method>::StreamDecryptor(ConstBuffer<uint8_t> key)
{
  assertTrue(key.size() == KEY_SIZE<method>, PichiError::CRYPTO_ERROR);
  // store the key in IV temporarily
  copy_n(cbegin(key), KEY_SIZE<method>, begin(iv_));
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
  assertFalse(initialized_, PichiError::MISC);
  assertTrue(iv.size() == IV_SIZE<method>, PichiError::CRYPTO_ERROR);
  auto key = array<uint8_t, KEY_SIZE<method>>{};
  copy_n(begin(iv_), KEY_SIZE<method>, begin(key));
  copy_n(cbegin(iv), IV_SIZE<method>, begin(iv_));
  initialize<method>(ctx_, key, {iv_, IV_SIZE<method>});
  initialized_ = true;
}

template <CryptoMethod method>
size_t StreamDecryptor<method>::decrypt(ConstBuffer<uint8_t> cipher, MutableBuffer<uint8_t> plain)
{
  assertTrue(initialized_, PichiError::MISC);
  offset_ = pichi::crypto::decrypt<method>(ctx_, offset_, cipher, iv_, plain);
  return cipher.size();
}

#if MBEDTLS_VERSION_MAJOR < 3
template class StreamDecryptor<CryptoMethod::RC4_MD5>;
template class StreamDecryptor<CryptoMethod::BF_CFB>;
#endif  // MBEDTLS_VERSION_MAJOR < 3
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

}  // namespace pichi::crypto
