#ifndef PICHI_CRYPTO_HASH_HPP
#define PICHI_CRYPTO_HASH_HPP

#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <pichi/common/buffer.hpp>
#include <pichi/common/enumerations.hpp>

#if MBEDTLS_VERSION_MAJOR >= 3
#define mbedtls_md5_starts_ret mbedtls_md5_starts
#define mbedtls_sha1_starts_ret mbedtls_sha1_starts
#define mbedtls_sha256_starts_ret mbedtls_sha256_starts
#define mbedtls_sha512_starts_ret mbedtls_sha512_starts
#define mbedtls_md5_update_ret mbedtls_md5_update
#define mbedtls_sha1_update_ret mbedtls_sha1_update
#define mbedtls_sha256_update_ret mbedtls_sha256_update
#define mbedtls_sha512_update_ret mbedtls_sha512_update
#define mbedtls_md5_finish_ret mbedtls_md5_finish
#define mbedtls_sha1_finish_ret mbedtls_sha1_finish
#define mbedtls_sha256_finish_ret mbedtls_sha256_finish
#define mbedtls_sha512_finish_ret mbedtls_sha512_finish
#endif  // MBEDTLS_VERSION_MAJOR >= 3

namespace pichi::crypto {

// TODO use lambda if compiler supports
struct StartFunctions {
  inline static int sha224(mbedtls_sha256_context* ctx)
  {
    return mbedtls_sha256_starts_ret(ctx, 1);
  }
  inline static int sha256(mbedtls_sha256_context* ctx)
  {
    return mbedtls_sha256_starts_ret(ctx, 0);
  }
  inline static int sha384(mbedtls_sha512_context* ctx)
  {
    return mbedtls_sha512_starts_ret(ctx, 1);
  }
  inline static int sha512(mbedtls_sha512_context* ctx)
  {
    return mbedtls_sha512_starts_ret(ctx, 0);
  }
};

template <HashAlgorithm algorithm> struct HashTraits {
};

template <> struct HashTraits<HashAlgorithm::MD5> {
  using Context = mbedtls_md5_context;
  static constexpr auto initialize = mbedtls_md5_init;
  static constexpr auto start = mbedtls_md5_starts_ret;
  static constexpr auto clone = mbedtls_md5_clone;
  static constexpr auto release = mbedtls_md5_free;
  static constexpr auto update = mbedtls_md5_update_ret;
  static constexpr auto finish = mbedtls_md5_finish_ret;
  static size_t const length = 16;
  static size_t const block_size = 64;
};

template <> struct HashTraits<HashAlgorithm::SHA1> {
  using Context = mbedtls_sha1_context;
  static constexpr auto initialize = mbedtls_sha1_init;
  static constexpr auto start = mbedtls_sha1_starts_ret;
  static constexpr auto clone = mbedtls_sha1_clone;
  static constexpr auto release = mbedtls_sha1_free;
  static constexpr auto update = mbedtls_sha1_update_ret;
  static constexpr auto finish = mbedtls_sha1_finish_ret;
  static size_t const length = 20;
  static size_t const block_size = 64;
};

template <> struct HashTraits<HashAlgorithm::SHA224> {
  using Context = mbedtls_sha256_context;
  static constexpr auto initialize = mbedtls_sha256_init;
  static constexpr auto start = StartFunctions::sha224;
  static constexpr auto clone = mbedtls_sha256_clone;
  static constexpr auto release = mbedtls_sha256_free;
  static constexpr auto update = mbedtls_sha256_update_ret;
  static constexpr auto finish = mbedtls_sha256_finish_ret;
  static size_t const length = 28;
  static size_t const block_size = 64;
};

template <> struct HashTraits<HashAlgorithm::SHA256> {
  using Context = mbedtls_sha256_context;
  static constexpr auto initialize = mbedtls_sha256_init;
  static constexpr auto start = StartFunctions::sha256;
  static constexpr auto clone = mbedtls_sha256_clone;
  static constexpr auto release = mbedtls_sha256_free;
  static constexpr auto update = mbedtls_sha256_update_ret;
  static constexpr auto finish = mbedtls_sha256_finish_ret;
  static size_t const length = 32;
  static size_t const block_size = 64;
};

template <> struct HashTraits<HashAlgorithm::SHA384> {
  using Context = mbedtls_sha512_context;
  static constexpr auto initialize = mbedtls_sha512_init;
  static constexpr auto start = StartFunctions::sha384;
  static constexpr auto clone = mbedtls_sha512_clone;
  static constexpr auto release = mbedtls_sha512_free;
  static constexpr auto update = mbedtls_sha512_update_ret;
  static constexpr auto finish = mbedtls_sha512_finish_ret;
  static size_t const length = 48;
  static size_t const block_size = 128;
};

template <> struct HashTraits<HashAlgorithm::SHA512> {
  using Context = mbedtls_sha512_context;
  static constexpr auto initialize = mbedtls_sha512_init;
  static constexpr auto start = StartFunctions::sha512;
  static constexpr auto clone = mbedtls_sha512_clone;
  static constexpr auto release = mbedtls_sha512_free;
  static constexpr auto update = mbedtls_sha512_update_ret;
  static constexpr auto finish = mbedtls_sha512_finish_ret;
  static size_t const length = 64;
  static size_t const block_size = 128;
};

template <HashAlgorithm algorithm> class Hash {
private:
  using Traits = HashTraits<algorithm>;
  using Context = typename Traits::Context;

public:
  Hash& operator=(Hash const&) = delete;
  Hash& operator=(Hash&&) = delete;

  Hash(Hash const&);
  Hash(Hash&&);

public:
  Hash();
  ~Hash();

  void append(ConstBuffer<uint8_t>);
  size_t hash(MutableBuffer<uint8_t>);
  size_t hash(ConstBuffer<uint8_t>, MutableBuffer<uint8_t>);

private:
  Context ctx_;
};

template <HashAlgorithm algorithm> class Hmac {
private:
  using H = Hash<algorithm>;
  using Traits = HashTraits<algorithm>;

public:
  Hmac& operator=(Hmac const&) = delete;
  Hmac& operator=(Hmac&&) = delete;

  Hmac(Hmac const&) = default;
  Hmac(Hmac&&) = default;

public:
  Hmac(ConstBuffer<uint8_t> key);
  void append(ConstBuffer<uint8_t>);
  size_t hash(MutableBuffer<uint8_t>);
  size_t hash(ConstBuffer<uint8_t>, MutableBuffer<uint8_t>);

private:
  H i_;
  H o_;
};

template <HashAlgorithm algorithm>
void hkdf(MutableBuffer<uint8_t> okm, ConstBuffer<uint8_t> ikm, ConstBuffer<uint8_t> salt,
          ConstBuffer<uint8_t> info = {(uint8_t const*)"ss-subkey", 9});

std::string bin2hex(ConstBuffer<uint8_t> bin);

}  // namespace pichi::crypto

#endif  // PICHI_CRYPTO_HASH_HPP
