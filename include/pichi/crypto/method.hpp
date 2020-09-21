#ifndef PICHI_CRYPTO_METHOD_HPP
#define PICHI_CRYPTO_METHOD_HPP

#include <pichi/common/enumerations.hpp>
#include <stddef.h>
#include <type_traits>

namespace pichi::crypto {

namespace detail {

template <CryptoMethod method> constexpr bool isArc() { return method == CryptoMethod::RC4_MD5; }

template <CryptoMethod method> constexpr bool isBlowfish()
{
  return method == CryptoMethod::BF_CFB;
}

template <CryptoMethod method> constexpr bool isAesCtr()
{
  switch (method) {
  case CryptoMethod::AES_128_CTR:
  case CryptoMethod::AES_192_CTR:
  case CryptoMethod::AES_256_CTR:
    return true;
  default:
    return false;
  }
}

template <CryptoMethod method> constexpr bool isAesCfb()
{
  switch (method) {
  case CryptoMethod::AES_128_CFB:
  case CryptoMethod::AES_192_CFB:
  case CryptoMethod::AES_256_CFB:
    return true;
  default:
    return false;
  }
}

template <CryptoMethod method> constexpr bool isCamellia()
{
  switch (method) {
  case CryptoMethod::CAMELLIA_128_CFB:
  case CryptoMethod::CAMELLIA_192_CFB:
  case CryptoMethod::CAMELLIA_256_CFB:
    return true;
  default:
    return false;
  }
}

template <CryptoMethod method> constexpr bool isGcm()
{
  switch (method) {
  case CryptoMethod::AES_128_GCM:
  case CryptoMethod::AES_192_GCM:
  case CryptoMethod::AES_256_GCM:
    return true;
  default:
    return false;
  }
}

template <CryptoMethod method> constexpr bool isSodiumStream()
{
  switch (method) {
  case CryptoMethod::CHACHA20:
  case CryptoMethod::SALSA20:
  case CryptoMethod::CHACHA20_IETF:
    return true;
  default:
    return false;
  }
}

template <CryptoMethod method> constexpr bool isSodiumAead()
{
  return method == CryptoMethod::CHACHA20_IETF_POLY1305 ||
         method == CryptoMethod::XCHACHA20_IETF_POLY1305;
}

template <CryptoMethod method> constexpr bool isStream()
{
  return isArc<method>() || isBlowfish<method>() || isAesCtr<method>() || isAesCfb<method>() ||
         isCamellia<method>() || isSodiumStream<method>();
}

template <CryptoMethod method> constexpr bool isAead()
{
  return isGcm<method>() || isSodiumAead<method>();
}

template <CryptoMethod method> struct DependentFalse : public std::false_type {
};

template <CryptoMethod method> constexpr size_t calcKeySize()
{
  static_assert(isStream<method>() || isAead<method>());
  switch (method) {
  case CryptoMethod::RC4_MD5:
  case CryptoMethod::BF_CFB:
  case CryptoMethod::AES_128_CTR:
  case CryptoMethod::AES_128_CFB:
  case CryptoMethod::CAMELLIA_128_CFB:
  case CryptoMethod::AES_128_GCM:
    return 16;
  case CryptoMethod::AES_192_CTR:
  case CryptoMethod::AES_192_CFB:
  case CryptoMethod::CAMELLIA_192_CFB:
  case CryptoMethod::AES_192_GCM:
    return 24;
  case CryptoMethod::AES_256_CTR:
  case CryptoMethod::AES_256_CFB:
  case CryptoMethod::AES_256_GCM:
  case CryptoMethod::CAMELLIA_256_CFB:
  case CryptoMethod::CHACHA20:
  case CryptoMethod::SALSA20:
  case CryptoMethod::CHACHA20_IETF:
  case CryptoMethod::CHACHA20_IETF_POLY1305:
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return 32;
  default:
    // Never goes here
    return 0;
  }
}

template <CryptoMethod method> constexpr size_t calcIvSize()
{
  if constexpr (method == CryptoMethod::CHACHA20_IETF)
    return 12;
  else if constexpr (method == CryptoMethod::BF_CFB || isSodiumStream<method>())
    return 8;
  else if constexpr (isStream<method>())
    return 16;
  else if constexpr (isAead<method>())
    return calcKeySize<method>();
  else
    static_assert(DependentFalse<method>::value);
}

template <CryptoMethod method> constexpr size_t calcNonceSize()
{
  static_assert(isAead<method>());
  return method == CryptoMethod::XCHACHA20_IETF_POLY1305 ? 24 : 12;
}

template <CryptoMethod method> constexpr size_t calcTagSize()
{
  static_assert(isAead<method>());
  return 16;
}

}  // namespace detail

template <CryptoMethod method> class StreamEncryptor;
template <CryptoMethod method> class AeadEncryptor;
template <CryptoMethod method>
using Encryptor =
    std::conditional_t<detail::isStream<method>(), StreamEncryptor<method>,
                       std::conditional_t<detail::isAead<method>(), AeadEncryptor<method>, void>>;

template <CryptoMethod method> class StreamDecryptor;
template <CryptoMethod method> class AeadDecryptor;
template <CryptoMethod method>
using Decryptor =
    std::conditional_t<detail::isStream<method>(), StreamDecryptor<method>,
                       std::conditional_t<detail::isAead<method>(), AeadDecryptor<method>, void>>;

template <CryptoMethod method> inline constexpr size_t KEY_SIZE = detail::calcKeySize<method>();
template <CryptoMethod method> inline constexpr size_t IV_SIZE = detail::calcIvSize<method>();
template <CryptoMethod method> inline constexpr size_t NONCE_SIZE = detail::calcNonceSize<method>();
template <CryptoMethod method> inline constexpr size_t TAG_SIZE = detail::calcTagSize<method>();

}  // namespace pichi::crypto

#endif
