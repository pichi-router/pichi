#include "pichi/common/config.hpp"
#include <pichi/stream/tls.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION >= 107300
#include <boost/asio/ssl/host_name_verification.hpp>
#else  // BOOST_VERSION >= 107300
#include <boost/asio/ssl/rfc2818_verification.hpp>
#endif  // BOOST_VERSION >= 107300

#ifdef TLS_FINGERPRINT
#include <array>
#include <brotli/decode.h>
#include <brotli/encode.h>
#include <pichi/common/literals.hpp>
#endif  // TLS_FINGERPRINT

namespace asio = boost::asio;
namespace ssl  = asio::ssl;

namespace pichi::stream {

#ifdef TLS_FINGERPRINT

void setup_fingerprint(::SSL* ssl)
{
  ::SSL_add_application_settings(ssl, reinterpret_cast<uint8_t const*>("h2"), 2, NULL, 0);
}

static int compress(SSL*, ::CBB* buf, uint8_t const* in, size_t in_len)
{
  auto out_len = ::BrotliEncoderMaxCompressedSize(in_len);
  auto out     = static_cast<uint8_t*>(nullptr);
  if (::CBB_reserve(buf, &out, out_len) == 0) {
    return 0;
  }
  if (::BrotliEncoderCompress(
          BROTLI_DEFAULT_QUALITY,
          BROTLI_DEFAULT_WINDOW,
          BROTLI_DEFAULT_MODE,
          in_len,
          in,
          &out_len,
          out
      ) != BROTLI_DECODER_RESULT_SUCCESS) {
    ::CBB_did_write(buf, 0_sz);
    return 0;
  }
  ::CBB_did_write(buf, out_len);
  return 1;
}

static int decompress(
    ::SSL*, ::CRYPTO_BUFFER** out, size_t uncompressed_len, uint8_t const* in, size_t in_len
)
{
  auto data = static_cast<uint8_t*>(nullptr);
  *out      = ::CRYPTO_BUFFER_alloc(&data, uncompressed_len);
  if (*out == nullptr) return 0;

  auto out_len = uncompressed_len;
  if (::BrotliDecoderDecompress(in_len, in, &out_len, data) != BROTLI_DECODER_RESULT_SUCCESS ||
      out_len != uncompressed_len) {
    ::CRYPTO_BUFFER_free(*out);
    return 0;
  }

  return 1;
}

// Simulate the TLS fingerprint according to https://tlsfingerprint.io/id/e47eae8f8c4887b6
static void setup_fingerprint(::SSL_CTX* ctx)
{
  static decltype(auto) CIPHER_SUITES =
      "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:"
      "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:"
      "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:"
      "ECDHE-RSA-AES128-SHA:ECDHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:"
      "AES128-SHA:AES256-SHA";

  static auto const ALPN = std::array{
      0x02_u8,
      0x68_u8,
      0x32_u8,
      0x08_u8,
      0x68_u8,
      0x74_u8,
      0x74_u8,
      0x70_u8,
      0x2f_u8,
      0x31_u8,
      0x2e_u8,
      0x31_u8
  };

  static auto const ALGORITHMS = std::array{
      NID_sha256,
      EVP_PKEY_EC,
      NID_sha256,
      EVP_PKEY_RSA_PSS,
      NID_sha256,
      EVP_PKEY_RSA,
      NID_sha384,
      EVP_PKEY_EC,
      NID_sha384,
      EVP_PKEY_RSA_PSS,
      NID_sha384,
      EVP_PKEY_RSA,
      NID_sha512,
      EVP_PKEY_RSA_PSS,
      NID_sha512,
      EVP_PKEY_RSA
  };

  ::SSL_CTX_set_permute_extensions(ctx, 1);
  ::SSL_CTX_set_grease_enabled(ctx, 1);
  ::SSL_CTX_enable_ocsp_stapling(ctx);
  ::SSL_CTX_set_cipher_list(ctx, CIPHER_SUITES);
  ::SSL_CTX_enable_signed_cert_timestamps(ctx);
  ::SSL_CTX_set_alpn_protos(ctx, ALPN.data(), static_cast<unsigned>(ALPN.size()));
  ::SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
  ::SSL_CTX_set1_sigalgs(ctx, ALGORITHMS.data(), ALGORITHMS.size());
  ::SSL_CTX_add_cert_compression_alg(ctx, TLSEXT_cert_compression_brotli, nullptr, &decompress);
}

#else   // TLS_FINGERPRINT
void setup_fingerprint(SSL*) {}
#endif  // TLS_FINGERPRINT

ssl::context tls_context(vo::TlsIngressOption const& opt)
{
  auto ctx = ssl::context{ssl::context::tls_server};
  ctx.use_certificate_chain_file(opt.certFile_);
  ctx.use_private_key_file(opt.keyFile_, ssl::context::pem);

#ifdef TLS_FINGERPRINT
  ::SSL_CTX_add_cert_compression_alg(
      ctx.native_handle(),
      TLSEXT_cert_compression_brotli,
      &compress,
      nullptr
  );
#endif  // TLS_FINGERPRINT

  return ctx;
}

ssl::context tls_context(vo::TlsEgressOption const& opt, std::string const& sn)
{
  auto ctx = ssl::context{ssl::context::tls_client};

#ifdef TLS_FINGERPRINT
  setup_fingerprint(ctx.native_handle());
#endif  // TLS_FINGERPRINT

  if (opt.insecure_) {
    ctx.set_verify_mode(ssl::context::verify_none);
    return ctx;
  }

  ctx.set_verify_mode(ssl::context::verify_peer);
  if (opt.caFile_.has_value())
    ctx.load_verify_file(*opt.caFile_);
  else {
    ctx.set_default_verify_paths();
#if BOOST_VERSION >= 107300
    ctx.set_verify_callback(ssl::host_name_verification{opt.serverName_.value_or(sn)});
#else   // BOOST_VERSION >= 107300
    ctx.set_verify_callback(ssl::rfc2818_verification{option.serverName_.value_or(serverName)});
#endif  // BOOST_VERSION >= 107300
  }
  return ctx;
}

}  // namespace pichi::stream
