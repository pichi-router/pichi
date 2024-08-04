#include <pichi/common/asserts.hpp>
#include <pichi/crypto/hash.hpp>
#include <pichi/crypto/key.hpp>

using namespace std;

namespace pichi::crypto {

template <CryptoMethod method> size_t generateKey(ConstBuffer pw, MutableBuffer psk)
{
  assertTrue(psk.size() >= KEY_SIZE<method>, PichiError::MISC);
  auto ct = ConstBuffer{};
  auto mt = MutableBuffer{psk, KEY_SIZE<method>};
  while (mt.size() != 0) {
    auto md5 = Hash<HashAlgorithm::MD5>{};
    md5.append(ct);
    md5.append(pw);
    auto len = md5.hash(mt);
    ct = {mt, len};
    mt = {mt.data() + len, mt.size() > len ? mt.size() - len : 0};
  }
  return KEY_SIZE<method>;
}

size_t generateKey(CryptoMethod method, ConstBuffer pw, MutableBuffer psk)
{
  switch (method) {
  case CryptoMethod::RC4_MD5:
    return generateKey<CryptoMethod::RC4_MD5>(pw, psk);
  case CryptoMethod::BF_CFB:
    return generateKey<CryptoMethod::BF_CFB>(pw, psk);
  case CryptoMethod::AES_128_CTR:
    return generateKey<CryptoMethod::AES_128_CTR>(pw, psk);
  case CryptoMethod::AES_192_CTR:
    return generateKey<CryptoMethod::AES_192_CTR>(pw, psk);
  case CryptoMethod::AES_256_CTR:
    return generateKey<CryptoMethod::AES_256_CTR>(pw, psk);
  case CryptoMethod::AES_128_CFB:
    return generateKey<CryptoMethod::AES_128_CFB>(pw, psk);
  case CryptoMethod::AES_192_CFB:
    return generateKey<CryptoMethod::AES_192_CFB>(pw, psk);
  case CryptoMethod::AES_256_CFB:
    return generateKey<CryptoMethod::AES_256_CFB>(pw, psk);
  case CryptoMethod::CAMELLIA_128_CFB:
    return generateKey<CryptoMethod::CAMELLIA_128_CFB>(pw, psk);
  case CryptoMethod::CAMELLIA_192_CFB:
    return generateKey<CryptoMethod::CAMELLIA_192_CFB>(pw, psk);
  case CryptoMethod::CAMELLIA_256_CFB:
    return generateKey<CryptoMethod::CAMELLIA_256_CFB>(pw, psk);
  case CryptoMethod::CHACHA20:
    return generateKey<CryptoMethod::CHACHA20>(pw, psk);
  case CryptoMethod::SALSA20:
    return generateKey<CryptoMethod::SALSA20>(pw, psk);
  case CryptoMethod::CHACHA20_IETF:
    return generateKey<CryptoMethod::CHACHA20_IETF>(pw, psk);
  case CryptoMethod::AES_128_GCM:
    return generateKey<CryptoMethod::AES_128_GCM>(pw, psk);
  case CryptoMethod::AES_192_GCM:
    return generateKey<CryptoMethod::AES_192_GCM>(pw, psk);
  case CryptoMethod::AES_256_GCM:
    return generateKey<CryptoMethod::AES_256_GCM>(pw, psk);
  case CryptoMethod::CHACHA20_IETF_POLY1305:
    return generateKey<CryptoMethod::CHACHA20_IETF_POLY1305>(pw, psk);
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return generateKey<CryptoMethod::XCHACHA20_IETF_POLY1305>(pw, psk);
  default:
    fail(PichiError::MISC);
  }
}

}  // namespace pichi::crypto
