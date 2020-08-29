#ifndef PICHI_CRYPTO_STREAM_HPP
#define PICHI_CRYPTO_STREAM_HPP

#include <algorithm>
#include <array>
#include <mbedtls/aes.h>
#include <mbedtls/arc4.h>
#include <mbedtls/blowfish.h>
#include <mbedtls/camellia.h>
#include <pichi/common/buffer.hpp>
#include <pichi/crypto/method.hpp>
#include <stdint.h>

namespace pichi::crypto {

template <CryptoMethod method>
using StreamContext = std::conditional_t<
    helpers::isArc<method>(), mbedtls_arc4_context,
    std::conditional_t<
        helpers::isBlowfish<method>(), mbedtls_blowfish_context,
        std::conditional_t<
            helpers::isAesCtr<method>() || helpers::isAesCfb<method>(), mbedtls_aes_context,
            std::conditional_t<helpers::isCamellia<method>(), mbedtls_camellia_context,
                               std::conditional_t<helpers::isSodiumStream<method>(),
                                                  std::array<uint8_t, KEY_SIZE<method>>, void>>>>>;

template <CryptoMethod method>
inline constexpr size_t BLK_SIZE = helpers::isAesCtr<method>() ? 16 : 0;

template <CryptoMethod method> class StreamEncryptor {
public:
  static_assert(helpers::isStream<method>(), "Not a stream crypto method");

  StreamEncryptor(StreamEncryptor const&) = delete;
  StreamEncryptor(StreamEncryptor&&) = delete;
  StreamEncryptor& operator=(StreamEncryptor const&) = delete;
  StreamEncryptor& operator=(StreamEncryptor&&) = delete;

public:
  explicit StreamEncryptor(ConstBuffer<uint8_t> key, ConstBuffer<uint8_t> iv = {});
  ~StreamEncryptor();

  ConstBuffer<uint8_t> getIv() const;
  size_t encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher);

private:
  StreamContext<method> ctx_;
  std::array<uint8_t, IV_SIZE<method> + BLK_SIZE<method>> iv_;
  size_t offset_ = 0;
};

template <CryptoMethod method> class StreamDecryptor {
public:
  static_assert(helpers::isStream<method>(), "Not a stream crypto method");

  StreamDecryptor(StreamDecryptor const&) = delete;
  StreamDecryptor(StreamDecryptor&&) = delete;
  StreamDecryptor& operator=(StreamDecryptor const&) = delete;
  StreamDecryptor& operator=(StreamDecryptor&&) = delete;

public:
  explicit StreamDecryptor(ConstBuffer<uint8_t> key);
  ~StreamDecryptor();

  size_t getIvSize() const;
  void setIv(ConstBuffer<uint8_t> iv);
  size_t decrypt(ConstBuffer<uint8_t> cipher, MutableBuffer<uint8_t> plain);

private:
  StreamContext<method> ctx_;
  std::array<uint8_t, std::max(IV_SIZE<method> + BLK_SIZE<method>, KEY_SIZE<method>)> iv_;
  size_t offset_ = 0;
  bool initialized_ = false;
};

}  // namespace pichi::crypto

#endif  // PICHI_CRYPTO_STREAM_HPP
