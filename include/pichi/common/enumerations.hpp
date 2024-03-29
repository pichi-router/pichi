#ifndef PICHI_COMMON_ENUMERATIONS_HPP
#define PICHI_COMMON_ENUMERATIONS_HPP

#ifdef _MSC_VER
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif  // TRANSPARENT
#endif  // _MSC_VER

namespace pichi {

enum class EndpointType { DOMAIN_NAME, IPV4, IPV6 };

enum class AdapterType { DIRECT, REJECT, SOCKS5, HTTP, SS, TUNNEL, TROJAN, VMESS, TRANSPARENT };

enum class HashAlgorithm { MD5, SHA1, SHA224, SHA256, SHA384, SHA512 };

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

enum class DelayMode { RANDOM, FIXED };
enum class BalanceType { RANDOM, ROUND_ROBIN, LEAST_CONN };
enum class VMessSecurity { AUTO, NONE, CHACHA20_IETF_POLY1305, AES_128_GCM };

enum class PichiError {
  OK = 0,
  BAD_PROTO,
  CRYPTO_ERROR,
  BUFFER_OVERFLOW,
  BAD_JSON,
  SEMANTIC_ERROR,
  RES_IN_USE,
  RES_LOCKED,
  CONN_FAILURE,
  BAD_AUTH_METHOD,
  UNAUTHENTICATED,
  MISC
};

}  // namespace pichi

#endif  // PICHI_COMMON_ENUMERATIONS_HPP
