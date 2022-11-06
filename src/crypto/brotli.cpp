#include <pichi/common/config.hpp>
// Include config.hpp first

#ifdef TLS_FINGERPRINT
#include <brotli/decode.h>
#endif  // TLS_FINGERPRINT

#include <openssl/ssl.h>
#include <pichi/crypto/brotli.hpp>

using namespace std;

namespace pichi::crypto {

#ifdef TLS_FINGERPRINT
int brotliDecompress(SSL*, CRYPTO_BUFFER** out, size_t uncompressed_len, uint8_t const* in,
                     size_t in_len)
{
  auto data = static_cast<uint8_t*>(nullptr);
  *out = CRYPTO_BUFFER_alloc(&data, uncompressed_len);
  if (*out == nullptr) return 0;

  auto out_len = uncompressed_len;
  if (BrotliDecoderDecompress(in_len, in, &out_len, data) != BROTLI_DECODER_RESULT_SUCCESS ||
      out_len != uncompressed_len) {
    CRYPTO_BUFFER_free(*out);
    return 0;
  }

  return 1;
}
#endif  // TLS_FINGERPRINT

}  // namespace pichi::crypto
