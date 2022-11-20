#ifndef PICHI_CRYPTO_FINGERPRINT_HPP
#define PICHI_CRYPTO_FINGERPRINT_HPP

#include <openssl/conf.h>

namespace pichi::crypto {

extern void enableBrotliCompression(::SSL_CTX*);
extern void setupTlsFingerprint(::SSL_CTX*);
extern void setupTlsFingerprint(::SSL*);

}  // namespace pichi::crypto

#endif  // PICHI_CRYPTO_FINGERPRINT_HPP
