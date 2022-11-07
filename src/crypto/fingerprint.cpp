#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <openssl/ssl.h>
#include <pichi/common/literals.hpp>
#include <pichi/crypto/fingerprint.hpp>

using namespace std;

namespace pichi::crypto {

// Simulate the TLS fingerprint according to https://tlsfingerprint.io/id/e47eae8f8c4887b6

#ifdef TLS_FINGERPRINT
void setupTlsFingerprint(::SSL_CTX* ctx)
{
  static decltype(auto) CIPHER_SUITES =
      "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:"
      "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:"
      "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:"
      "ECDHE-RSA-AES128-SHA:ECDHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:"
      "AES128-SHA:AES256-SHA";

  static auto const ALPN = array{0x02_u8, 0x68_u8, 0x32_u8, 0x08_u8, 0x68_u8, 0x74_u8,
                                 0x74_u8, 0x70_u8, 0x2f_u8, 0x31_u8, 0x2e_u8, 0x31_u8};

  static auto const ALGORITHMS =
      array{NID_sha256, EVP_PKEY_EC,      NID_sha256, EVP_PKEY_RSA_PSS, NID_sha256, EVP_PKEY_RSA,
            NID_sha384, EVP_PKEY_EC,      NID_sha384, EVP_PKEY_RSA_PSS, NID_sha384, EVP_PKEY_RSA,
            NID_sha512, EVP_PKEY_RSA_PSS, NID_sha512, EVP_PKEY_RSA};

  ::SSL_CTX_set_grease_enabled(ctx, 1);
  ::SSL_CTX_enable_ocsp_stapling(ctx);
  ::SSL_CTX_set_cipher_list(ctx, CIPHER_SUITES);
  ::SSL_CTX_enable_signed_cert_timestamps(ctx);
  ::SSL_CTX_set_alpn_protos(ctx, ALPN.data(), ALPN.size());
  ::SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
  ::SSL_CTX_set1_sigalgs(ctx, ALGORITHMS.data(), ALGORITHMS.size());
  ::SSL_CTX_add_cert_compression_alg(ctx, TLSEXT_cert_compression_brotli, nullptr,
                                     // Decompression stub
                                     [](auto, auto, auto, auto, auto) { return 0; });
#else   // ENABLE_TLS_FINGERPRINT
void setupTlsFingerprint(::SSL_CTX*)
{
#endif  // TLS_FINGERPRINT
}

#ifdef TLS_FINGERPRINT
void setupTlsFingerprint(::SSL* ssl)
{
  ::SSL_add_application_settings(ssl, reinterpret_cast<uint8_t const*>("h2"), 2, NULL, 0);
#else   // TLS_FINGERPRINT
void setupTlsFingerprint(::SSL*)
{
#endif  // TLS_FINGERPRINT
}

}  // namespace pichi::crypto
