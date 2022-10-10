#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <openssl/ssl.h>
#include <pichi/crypto/brotli.hpp>

using namespace std;

namespace pichi::crypto {

// #ifdef ENABLE_TLS_FINGERPRINT
#if 0
int brotliDecompress(SSL* ssl, CRYPTO_BUFFER** out, size_t uncompressed_len, uint8_t const* in,
                     size_t in_len)
{
  auto data = static_cast<uint8_t*>(nullptr);
  auto decompressed = make_unique<::CRYPTO_BUFFER>(CRYPTO_BUFFER_alloc(&data, uncompressed_len));
  if (decompressed == nullptr) return 0;

  auto out_len = uncompressed_len;
  if (BrotliDecodeDecompress(in_len, in, &out_len, data) != BROTLI_DECODER_RESULT_SUCCESS ||
      out_len != uncompressed_len)
    return 0;

  *out = decompressed.release();
  return 1;
}
#endif  // ENABLE_TLS_FINGERPRINT

}  // namespace pichi::crypto
