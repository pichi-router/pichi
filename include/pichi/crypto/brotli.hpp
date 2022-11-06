#ifndef PICHI_CRYPTO_FINGERPRINT_HPP
#define PICHI_CRYPTO_FINGERPRINT_HPP

#include <openssl/conf.h>

struct crypto_buffer_st;

namespace pichi::crypto {

int brotliDecompress(SSL*, crypto_buffer_st**, size_t, uint8_t const*, size_t);

}  // namespace pichi::crypto

#endif  // PICHI_CRYPTO_FINGERPRINT_HPP
