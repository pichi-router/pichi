#ifndef PICHI_COMMON_ENUMS_HPP
#define PICHI_COMMON_ENUMS_HPP

namespace pichi {

enum class AdapterType { DIRECT, REJECT, SOCKS5, HTTP, SS, TUNNEL };

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

#endif  // PICHI_COMMON_ENUMS_HPP
