#ifndef PICHI_CRYPTO_STREAM_HPP
#define PICHI_CRYPTO_STREAM_HPP

#include <algorithm>
#include <array>
#include <mbedtls/aes.h>
#include <mbedtls/camellia.h>
#include <pichi/common/buffer.hpp>
#include <pichi/crypto/method.hpp>
#include <stdint.h>

#if MBEDTLS_VERSION_MAJOR < 3
#include <mbedtls/arc4.h>
#include <mbedtls/blowfish.h>
#endif  // MBEDTLS_VERSION_MAJOR < 3

namespace pichi::crypto {

template <CryptoMethod method>
using StreamContext = std::conditional_t<
#if MBEDTLS_VERSION_MAJOR < 3
    detail::isArc<method>(), mbedtls_arc4_context,
    std::conditional_t<
        detail::isBlowfish<method>(), mbedtls_blowfish_context,
        std::conditional_t<
#endif  // MBEDTLS_VERSION_MAJOR < 3
            detail::isAesCtr<method>() || detail::isAesCfb<method>(), mbedtls_aes_context,
            std::conditional_t<detail::isCamellia<method>(), mbedtls_camellia_context,
                               std::conditional_t<detail::isSodiumStream<method>(),
                                                  std::array<uint8_t, KEY_SIZE<method>>, void>>>
#if MBEDTLS_VERSION_MAJOR < 3
        >>
#endif  // MBEDTLS_VERSION_MAJOR < 3
    ;

template <CryptoMethod method>
inline constexpr size_t BLK_SIZE = detail::isAesCtr<method>() ? 16 : 0;

template <CryptoMethod method> class StreamEncryptor {
public:
  static_assert(detail::isStream<method>(), "Not a stream crypto method");

  StreamEncryptor(StreamEncryptor const&) = delete;
  StreamEncryptor(StreamEncryptor&&) = delete;
  StreamEncryptor& operator=(StreamEncryptor const&) = delete;
  StreamEncryptor& operator=(StreamEncryptor&&) = delete;

public:
  explicit StreamEncryptor(ConstBuffer key, ConstBuffer iv = {});
  ~StreamEncryptor();

  ConstBuffer getIv() const;
  size_t encrypt(ConstBuffer plain, MutableBuffer cipher);

private:
  StreamContext<method> ctx_;
  std::array<uint8_t, IV_SIZE<method> + BLK_SIZE<method>> iv_;
  size_t offset_ = 0;
};

template <CryptoMethod method> class StreamDecryptor {
public:
  static_assert(detail::isStream<method>(), "Not a stream crypto method");

  StreamDecryptor(StreamDecryptor const&) = delete;
  StreamDecryptor(StreamDecryptor&&) = delete;
  StreamDecryptor& operator=(StreamDecryptor const&) = delete;
  StreamDecryptor& operator=(StreamDecryptor&&) = delete;

public:
  explicit StreamDecryptor(ConstBuffer key);
  ~StreamDecryptor();

  size_t getIvSize() const;
  void setIv(ConstBuffer iv);
  size_t decrypt(ConstBuffer cipher, MutableBuffer plain);

private:
  StreamContext<method> ctx_;
  std::array<uint8_t, std::max(IV_SIZE<method> + BLK_SIZE<method>, KEY_SIZE<method>)> iv_;
  size_t offset_ = 0;
  bool initialized_ = false;
};

}  // namespace pichi::crypto

#endif  // PICHI_CRYPTO_STREAM_HPP
