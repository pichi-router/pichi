#ifndef PICHI_CRYPTO_FINGERPRINT_HPP
#define PICHI_CRYPTO_FINGERPRINT_HPP

#include <openssl/conf.h>

namespace pichi::crypto {

template <typename CBB> int brotliCompress(SSL*, CBB*, uint8_t const*, size_t);
template <typename CRYPTO_BUFFER>
int brotliDecompress(SSL*, CRYPTO_BUFFER**, size_t, uint8_t const*, size_t);

}  // namespace pichi::crypto

#endif  // PICHI_CRYPTO_FINGERPRINT_HPP
