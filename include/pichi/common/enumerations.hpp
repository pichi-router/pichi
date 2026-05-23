#ifndef PICHI_COMMON_ENUMERATIONS_HPP
#define PICHI_COMMON_ENUMERATIONS_HPP

#ifdef _WIN32
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif  // TRANSPARENT
#endif  // _WIN32

namespace pichi {

enum class EndpointType { DOMAIN_NAME, IPV4, IPV6 };

enum class AdapterType { DIRECT, REJECT, SOCKS5, HTTP, SS, TUNNEL, TROJAN, TRANSPARENT };

enum class CryptoMethod {
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
