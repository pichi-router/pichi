#include <pichi/common/config.hpp>
// Include config.hpp first

#ifdef TLS_FINGERPRINT
#include <brotli/decode.h>
#include <brotli/encode.h>
#include <openssl/bytestring.h>
#endif  // TLS_FINGERPRINT

#include <openssl/ssl.h>
#include <pichi/common/literals.hpp>
#include <pichi/crypto/brotli.hpp>

using namespace std;

namespace pichi::crypto {

#ifdef TLS_FINGERPRINT

template <typename CBB> int brotliCompress(SSL*, CBB* buf, uint8_t const* in, size_t in_len)
{
  auto out_len = BrotliEncoderMaxCompressedSize(in_len);
  auto out = static_cast<uint8_t*>(nullptr);
  if (CBB_reserve(buf, &out, out_len) == 0) {
    return 0;
  }
  if (BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE,
                            in_len, in, &out_len, out) != BROTLI_DECODER_RESULT_SUCCESS) {
    CBB_did_write(buf, 0_sz);
    return 0;
  }
  CBB_did_write(buf, out_len);
  return 1;
}

template <typename CRYPTO_BUFFER>
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

template int brotliCompress<CBB>(SSL*, CBB*, uint8_t const*, size_t);
template int brotliDecompress<CRYPTO_BUFFER>(SSL*, CRYPTO_BUFFER**, size_t, uint8_t const*, size_t);

#endif  // TLS_FINGERPRINT

}  // namespace pichi::crypto
