#include <array>
#include <iostream>
#include <pichi/asserts.hpp>
#include <pichi/crypto/hash.hpp>
#include <pichi/crypto/stream.hpp>
#include <sodium.h>

using namespace std;

namespace pichi {
namespace crypto {

struct Initializations {
  static void aesCfb(mbedtls_aes_context& ctx, ConstBuffer<uint8_t> key, vector<uint8_t> const& iv,
                     vector<uint8_t>& block, size_t blen)
  {
    mbedtls_aes_init(&ctx);
    auto ret = mbedtls_aes_setkey_enc(&ctx, key.data(), key.size() * 8);
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void aesCtr(mbedtls_aes_context& ctx, ConstBuffer<uint8_t> key, vector<uint8_t> const& iv,
                     vector<uint8_t>& block, size_t blen)
  {
    block.resize(blen, 0);
    mbedtls_aes_init(&ctx);
    auto ret = mbedtls_aes_setkey_enc(&ctx, key.data(), key.size() * 8);
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void arc4(mbedtls_arc4_context& ctx, ConstBuffer<uint8_t> key, vector<uint8_t> const& iv,
                   vector<uint8_t>& block, size_t blen)
  {
    auto md5 = Hash<HashAlgorithm::MD5>{};
    md5.append(key);
    md5.append(iv);

    auto skey = array<uint8_t, HashTraits<HashAlgorithm::MD5>::length>{};
    md5.hash(skey);

    mbedtls_arc4_init(&ctx);
    mbedtls_arc4_setup(&ctx, skey.data(), skey.size());
  }

  static void blowfish(mbedtls_blowfish_context& ctx, ConstBuffer<uint8_t> key,
                       vector<uint8_t> const& iv, vector<uint8_t>& block, size_t blen)
  {
    mbedtls_blowfish_init(&ctx);
    auto ret = mbedtls_blowfish_setkey(&ctx, key.data(), key.size() * 8);
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void camellia(mbedtls_camellia_context& ctx, ConstBuffer<uint8_t> key,
                       vector<uint8_t> const& iv, vector<uint8_t>& block, size_t blen)
  {
    mbedtls_camellia_init(&ctx);
    auto ret = mbedtls_camellia_setkey_enc(&ctx, key.data(), key.size() * 8);
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  }

  static void sodium(vector<uint8_t>& ctx, ConstBuffer<uint8_t> key, vector<uint8_t> const& iv,
                     vector<uint8_t>& block, size_t blen)
  {
    ctx.assign(key.begin(), key.end());
  }
};

struct Releasings {
  static void aes(mbedtls_aes_context& ctx) { mbedtls_aes_free(&ctx); }

  static void arc4(mbedtls_arc4_context& ctx) { mbedtls_arc4_free(&ctx); }

  static void blowfish(mbedtls_blowfish_context& ctx) { mbedtls_blowfish_free(&ctx); }

  static void camellia(mbedtls_camellia_context& ctx) { mbedtls_camellia_free(&ctx); }

  static void sodium(vector<uint8_t>& ctx) {}
};

template <typename SodiumFunc>
size_t sodiumHelper(SodiumFunc&& func, ConstBuffer<uint8_t> key, size_t offset,
                    ConstBuffer<uint8_t> input, MutableBuffer<uint8_t> iv,
                    MutableBuffer<uint8_t> block, size_t blen, MutableBuffer<uint8_t> output)
{
  auto padding = offset % blen;
  auto ib = vector<uint8_t>(input.size() + padding, 0);
  auto ob = vector<uint8_t>(input.size() + padding, 0);

  copy(input.begin(), input.end(), ib.begin() + padding);
  auto ret = func(ob.data(), ib.data(), ib.size(), iv.data(), offset / blen, key.data());
  assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
  copy(ob.begin() + padding, ob.end(), output.begin());

  return offset + input.size();
}

struct Encryptions {
  static size_t aesCfb(mbedtls_aes_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                       MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                       MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_ENCRYPT, input.size(), &offset, iv.data(),
                                        input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset;
  }

  static size_t aesCtr(mbedtls_aes_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                       MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                       MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_aes_crypt_ctr(&ctx, input.size(), &offset, iv.data(), block.data(),
                                     input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset;
  }

  static size_t arc4(mbedtls_arc4_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                     MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                     MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_arc4_crypt(&ctx, input.size(), input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset + input.size();
  }

  static size_t blowfish(mbedtls_blowfish_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                         MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                         MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_blowfish_crypt_cfb64(&ctx, MBEDTLS_BLOWFISH_ENCRYPT, input.size(), &offset,
                                            iv.data(), input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset;
  }

  static size_t camellia(mbedtls_camellia_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                         MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                         MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_camellia_crypt_cfb128(&ctx, MBEDTLS_CAMELLIA_ENCRYPT, input.size(), &offset,
                                             iv.data(), input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset;
  }

  static size_t chacha20(ConstBuffer<uint8_t> key, size_t offset, ConstBuffer<uint8_t> input,
                         MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                         MutableBuffer<uint8_t> output)
  {
    return sodiumHelper(crypto_stream_chacha20_xor_ic, key, offset, input, iv, block, blen, output);
  }

  static size_t salsa20(ConstBuffer<uint8_t> key, size_t offset, ConstBuffer<uint8_t> input,
                        MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                        MutableBuffer<uint8_t> output)
  {
    return sodiumHelper(crypto_stream_salsa20_xor_ic, key, offset, input, iv, block, blen, output);
  }

  static size_t chacha20Ietf(ConstBuffer<uint8_t> key, size_t offset, ConstBuffer<uint8_t> input,
                             MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                             MutableBuffer<uint8_t> output)
  {
    return sodiumHelper(crypto_stream_chacha20_ietf_xor_ic, key, offset, input, iv, block, blen,
                        output);
  }
};

struct Decryptions {
  static size_t aesCfb(mbedtls_aes_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                       MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                       MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_aes_crypt_cfb128(&ctx, MBEDTLS_AES_DECRYPT, input.size(), &offset, iv.data(),
                                        input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset;
  }

  static size_t aesCtr(mbedtls_aes_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                       MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                       MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_aes_crypt_ctr(&ctx, input.size(), &offset, iv.data(), block.data(),
                                     input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset;
  }

  static size_t arc4(mbedtls_arc4_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                     MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                     MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_arc4_crypt(&ctx, input.size(), input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset + input.size();
  }

  static size_t blowfish(mbedtls_blowfish_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                         MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                         MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_blowfish_crypt_cfb64(&ctx, MBEDTLS_BLOWFISH_DECRYPT, input.size(), &offset,
                                            iv.data(), input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset;
  }

  static size_t camellia(mbedtls_camellia_context& ctx, size_t offset, ConstBuffer<uint8_t> input,
                         MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                         MutableBuffer<uint8_t> output)
  {
    auto ret = mbedtls_camellia_crypt_cfb128(&ctx, MBEDTLS_CAMELLIA_DECRYPT, input.size(), &offset,
                                             iv.data(), input.data(), output.data());
    assertTrue(ret == 0, PichiError::CRYPTO_ERROR);
    return offset;
  }

  static size_t chacha20(ConstBuffer<uint8_t> key, size_t offset, ConstBuffer<uint8_t> input,
                         MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                         MutableBuffer<uint8_t> output)
  {
    return sodiumHelper(crypto_stream_chacha20_xor_ic, key, offset, input, iv, block, blen, output);
  }

  static size_t salsa20(ConstBuffer<uint8_t> key, size_t offset, ConstBuffer<uint8_t> input,
                        MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                        MutableBuffer<uint8_t> output)
  {
    return sodiumHelper(crypto_stream_salsa20_xor_ic, key, offset, input, iv, block, blen, output);
  }

  static size_t chacha20Ietf(ConstBuffer<uint8_t> key, size_t offset, ConstBuffer<uint8_t> input,
                             MutableBuffer<uint8_t> iv, MutableBuffer<uint8_t> block, size_t blen,
                             MutableBuffer<uint8_t> output)
  {
    return sodiumHelper(crypto_stream_chacha20_ietf_xor_ic, key, offset, input, iv, block, blen,
                        output);
  }
};

template <CryptoMethod method> struct StreamTrait {
};

template <> struct StreamTrait<CryptoMethod::RC4_MD5> {
  static const size_t block_size = 0;
  static constexpr auto initialize = Initializations::arc4;
  static constexpr auto release = Releasings::arc4;
  static constexpr auto encrypt = Encryptions::arc4;
  static constexpr auto decrypt = Decryptions::arc4;
};

template <> struct StreamTrait<CryptoMethod::BF_CFB> {
  static const size_t block_size = 0;
  static constexpr auto initialize = Initializations::blowfish;
  static constexpr auto release = Releasings::blowfish;
  static constexpr auto encrypt = Encryptions::blowfish;
  static constexpr auto decrypt = Decryptions::blowfish;
};

template <> struct StreamTrait<CryptoMethod::AES_128_CTR> {
  static const size_t block_size = 16;
  static constexpr auto initialize = Initializations::aesCtr;
  static constexpr auto release = Releasings::aes;
  static constexpr auto encrypt = Encryptions::aesCtr;
  static constexpr auto decrypt = Decryptions::aesCtr;
};

template <> struct StreamTrait<CryptoMethod::AES_192_CTR> {
  static const size_t block_size = 16;
  static constexpr auto initialize = Initializations::aesCtr;
  static constexpr auto release = Releasings::aes;
  static constexpr auto encrypt = Encryptions::aesCtr;
  static constexpr auto decrypt = Decryptions::aesCtr;
};

template <> struct StreamTrait<CryptoMethod::AES_256_CTR> {
  static const size_t block_size = 16;
  static constexpr auto initialize = Initializations::aesCtr;
  static constexpr auto release = Releasings::aes;
  static constexpr auto encrypt = Encryptions::aesCtr;
  static constexpr auto decrypt = Decryptions::aesCtr;
};

template <> struct StreamTrait<CryptoMethod::AES_128_CFB> {
  static const size_t block_size = 0;
  static constexpr auto initialize = Initializations::aesCfb;
  static constexpr auto release = Releasings::aes;
  static constexpr auto encrypt = Encryptions::aesCfb;
  static constexpr auto decrypt = Decryptions::aesCfb;
};

template <> struct StreamTrait<CryptoMethod::AES_192_CFB> {
  static const size_t block_size = 0;
  static constexpr auto initialize = Initializations::aesCfb;
  static constexpr auto release = Releasings::aes;
  static constexpr auto encrypt = Encryptions::aesCfb;
  static constexpr auto decrypt = Decryptions::aesCfb;
};

template <> struct StreamTrait<CryptoMethod::AES_256_CFB> {
  static const size_t block_size = 0;
  static constexpr auto initialize = Initializations::aesCfb;
  static constexpr auto release = Releasings::aes;
  static constexpr auto encrypt = Encryptions::aesCfb;
  static constexpr auto decrypt = Decryptions::aesCfb;
};

template <> struct StreamTrait<CryptoMethod::CAMELLIA_128_CFB> {
  static const size_t block_size = 0;
  static constexpr auto initialize = Initializations::camellia;
  static constexpr auto release = Releasings::camellia;
  static constexpr auto encrypt = Encryptions::camellia;
  static constexpr auto decrypt = Decryptions::camellia;
};

template <> struct StreamTrait<CryptoMethod::CAMELLIA_192_CFB> {
  static const size_t block_size = 0;
  static constexpr auto initialize = Initializations::camellia;
  static constexpr auto release = Releasings::camellia;
  static constexpr auto encrypt = Encryptions::camellia;
  static constexpr auto decrypt = Decryptions::camellia;
};

template <> struct StreamTrait<CryptoMethod::CAMELLIA_256_CFB> {
  static const size_t block_size = 0;
  static constexpr auto initialize = Initializations::camellia;
  static constexpr auto release = Releasings::camellia;
  static constexpr auto encrypt = Encryptions::camellia;
  static constexpr auto decrypt = Decryptions::camellia;
};

template <> struct StreamTrait<CryptoMethod::CHACHA20> {
  static size_t const block_size = 64;
  static constexpr auto initialize = Initializations::sodium;
  static constexpr auto release = Releasings::sodium;
  static constexpr auto encrypt = Encryptions::chacha20;
  static constexpr auto decrypt = Decryptions::chacha20;
};

template <> struct StreamTrait<CryptoMethod::SALSA20> {
  static size_t const block_size = 64;
  static constexpr auto initialize = Initializations::sodium;
  static constexpr auto release = Releasings::sodium;
  static constexpr auto encrypt = Encryptions::salsa20;
  static constexpr auto decrypt = Decryptions::salsa20;
};

template <> struct StreamTrait<CryptoMethod::CHACHA20_IETF> {
  static size_t const block_size = 64;
  static constexpr auto initialize = Initializations::sodium;
  static constexpr auto release = Releasings::sodium;
  static constexpr auto encrypt = Encryptions::chacha20Ietf;
  static constexpr auto decrypt = Decryptions::chacha20Ietf;
};

template <CryptoMethod method>
StreamEncryptor<method>::StreamEncryptor(ConstBuffer<uint8_t> key, ConstBuffer<uint8_t> iv)
  : iv_{iv.begin(), iv.end()}
{
  if (iv_.empty()) {
    iv_.resize(CryptoLength<method>::IV);
    randombytes_buf(iv_.data(), iv_.size());
  }
  assertTrue(iv_.size() == CryptoLength<method>::IV, PichiError::CRYPTO_ERROR);
  StreamTrait<method>::initialize(ctx_, key, iv_, block_, StreamTrait<method>::block_size);
}

template <CryptoMethod method> StreamEncryptor<method>::~StreamEncryptor()
{
  StreamTrait<method>::release(ctx_);
}

template <CryptoMethod method> ConstBuffer<uint8_t> StreamEncryptor<method>::getIv() const
{
  return {iv_};
}

template <CryptoMethod method>
size_t StreamEncryptor<method>::encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher)
{
  assertTrue(cipher.size() >= plain.size(), PichiError::CRYPTO_ERROR);
  offset_ = StreamTrait<method>::encrypt(ctx_, offset_, plain, iv_, block_,
                                         StreamTrait<method>::block_size, cipher);
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

template <CryptoMethod method> StreamDecryptor<method>::StreamDecryptor(ConstBuffer<uint8_t> key)
{
  block_.assign(key.begin(), key.end());
}

template <CryptoMethod method> StreamDecryptor<method>::~StreamDecryptor()
{
  StreamTrait<method>::release(ctx_);
}

template <CryptoMethod method> size_t StreamDecryptor<method>::getIvSize() const
{
  return CryptoLength<method>::IV;
}

template <CryptoMethod method> void StreamDecryptor<method>::setIv(ConstBuffer<uint8_t> iv)
{
  assertTrue(iv_.empty(), PichiError::CRYPTO_ERROR);
  assertTrue(iv.size() == CryptoLength<method>::IV, PichiError::CRYPTO_ERROR);
  iv_.assign(iv.begin(), iv.end());
  auto key = move(block_);
  StreamTrait<method>::initialize(ctx_, key, iv_, block_, StreamTrait<method>::block_size);
}

template <CryptoMethod method>
size_t StreamDecryptor<method>::decrypt(ConstBuffer<uint8_t> cipher, MutableBuffer<uint8_t> plain)
{
  assertTrue(iv_.size() == CryptoLength<method>::IV, PichiError::CRYPTO_ERROR);
  offset_ = StreamTrait<method>::decrypt(ctx_, offset_, cipher, iv_, block_,
                                         StreamTrait<method>::block_size, plain);
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

} // namespace crypto
} // namespace pichi
