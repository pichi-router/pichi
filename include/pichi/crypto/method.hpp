#ifndef PICHI_CRYPTO_METHOD_HPP
#define PICHI_CRYPTO_METHOD_HPP

#include <stddef.h>
#include <type_traits>

namespace pichi {
namespace crypto {

enum class CryptoMethod {
  // Stream Crypto Method begin
  // Misc methods
  RC4_MD5,
  BF_CFB,

  // AES-CTR methods
  AES_128_CTR,
  AES_192_CTR,
  AES_256_CTR,

  // AES-CFB methods
  AES_128_CFB,
  AES_192_CFB,
  AES_256_CFB,

  // CAMELLIA-CFB methods
  CAMELLIA_128_CFB,
  CAMELLIA_192_CFB,
  CAMELLIA_256_CFB,

  // CHACHA & SALSA methods
  CHACHA20,
  SALSA20,
  CHACHA20_IETF,
  // Stream Crypto Method end

  // AEAD Crypto Method begin
  // AES-GCM methods
  AES_128_GCM,
  AES_192_GCM,
  AES_256_GCM,

  // CHACHA20 methods
  CHACHA20_IETF_POLY1305,
  XCHACHA20_IETF_POLY1305
  // AEAD Crypto Method end
};

constexpr bool isStream(CryptoMethod method)
{
  switch (method) {
  case CryptoMethod::RC4_MD5:
  case CryptoMethod::BF_CFB:
  case CryptoMethod::AES_128_CTR:
  case CryptoMethod::AES_192_CTR:
  case CryptoMethod::AES_256_CTR:
  case CryptoMethod::AES_128_CFB:
  case CryptoMethod::AES_192_CFB:
  case CryptoMethod::AES_256_CFB:
  case CryptoMethod::CAMELLIA_128_CFB:
  case CryptoMethod::CAMELLIA_192_CFB:
  case CryptoMethod::CAMELLIA_256_CFB:
  case CryptoMethod::CHACHA20:
  case CryptoMethod::SALSA20:
  case CryptoMethod::CHACHA20_IETF:
    return true;
  default:
    return false;
  }
}

constexpr bool isAead(CryptoMethod method)
{
  switch (method) {
  case CryptoMethod::AES_128_GCM:
  case CryptoMethod::AES_192_GCM:
  case CryptoMethod::AES_256_GCM:
  case CryptoMethod::CHACHA20_IETF_POLY1305:
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return true;
  default:

    return false;
  }
}

constexpr bool isArc(CryptoMethod method) { return method == CryptoMethod::RC4_MD5; }

constexpr bool isBlowfish(CryptoMethod method) { return method == CryptoMethod::BF_CFB; }

constexpr bool isAes(CryptoMethod method)
{
  switch (method) {
  case CryptoMethod::AES_128_CTR:
  case CryptoMethod::AES_192_CTR:
  case CryptoMethod::AES_256_CTR:
  case CryptoMethod::AES_128_CFB:
  case CryptoMethod::AES_192_CFB:
  case CryptoMethod::AES_256_CFB:
    return true;
  default:
    return false;
  }
}

constexpr bool isCamellia(CryptoMethod method)
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

constexpr bool isGcm(CryptoMethod method)
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

constexpr bool isSodiumStream(CryptoMethod method)
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

constexpr bool isSodiumAead(CryptoMethod method)
{
  return method == CryptoMethod::CHACHA20_IETF_POLY1305 ||
         method == CryptoMethod::XCHACHA20_IETF_POLY1305;
}

template <CryptoMethod method> class StreamEncryptor;
template <CryptoMethod method> class AeadEncryptor;
template <CryptoMethod method>
using Encryptor =
    std::conditional_t<isStream(method), StreamEncryptor<method>,
                       std::conditional_t<isAead(method), AeadEncryptor<method>, void>>;

template <CryptoMethod method> class StreamDecryptor;
template <CryptoMethod method> class AeadDecryptor;
template <CryptoMethod method>
using Decryptor =
    std::conditional_t<isStream(method), StreamDecryptor<method>,
                       std::conditional_t<isAead(method), AeadDecryptor<method>, void>>;

template <CryptoMethod method> struct CryptoLength {
};

template <> struct CryptoLength<CryptoMethod::RC4_MD5> {
  static size_t const KEY = 16;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::BF_CFB> {
  static size_t const KEY = 16;
  static size_t const IV = 8;
};

template <> struct CryptoLength<CryptoMethod::AES_128_CTR> {
  static size_t const KEY = 16;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::AES_192_CTR> {
  static size_t const KEY = 24;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::AES_256_CTR> {
  static size_t const KEY = 32;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::AES_128_CFB> {
  static size_t const KEY = 16;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::AES_192_CFB> {
  static size_t const KEY = 24;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::AES_256_CFB> {
  static size_t const KEY = 32;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::CAMELLIA_128_CFB> {
  static size_t const KEY = 16;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::CAMELLIA_192_CFB> {
  static size_t const KEY = 24;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::CAMELLIA_256_CFB> {
  static size_t const KEY = 32;
  static size_t const IV = 16;
};

template <> struct CryptoLength<CryptoMethod::CHACHA20> {
  static size_t const KEY = 32;
  static size_t const IV = 8;
};

template <> struct CryptoLength<CryptoMethod::SALSA20> {
  static size_t const KEY = 32;
  static size_t const IV = 8;
};

template <> struct CryptoLength<CryptoMethod::CHACHA20_IETF> {
  static size_t const KEY = 32;
  static size_t const IV = 12;
};

template <> struct CryptoLength<CryptoMethod::AES_128_GCM> {
  static size_t const KEY = 16;
  static size_t const IV = KEY;
  static size_t const NONCE = 12;
  static size_t const TAG = 16;
};

template <> struct CryptoLength<CryptoMethod::AES_192_GCM> {
  static size_t const KEY = 24;
  static size_t const IV = KEY;
  static size_t const NONCE = 12;
  static size_t const TAG = 16;
};

template <> struct CryptoLength<CryptoMethod::AES_256_GCM> {
  static size_t const KEY = 32;
  static size_t const IV = KEY;
  static size_t const NONCE = 12;
  static size_t const TAG = 16;
};

template <> struct CryptoLength<CryptoMethod::CHACHA20_IETF_POLY1305> {
  static size_t const KEY = 32;
  static size_t const IV = KEY;
  static size_t const NONCE = 12;
  static size_t const TAG = 16;
};

template <> struct CryptoLength<CryptoMethod::XCHACHA20_IETF_POLY1305> {
  static size_t const KEY = 32;
  static size_t const IV = KEY;
  static size_t const NONCE = 24;
  static size_t const TAG = 16;
};

} // namespace crypto
} // namespace pichi

#endif
