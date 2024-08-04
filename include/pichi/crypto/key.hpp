#ifndef PICHI_CRYPTO_KEY_HPP
#define PICHI_CRYPTO_KEY_HPP

#include <pichi/common/buffer.hpp>
#include <pichi/crypto/method.hpp>
#include <span>
#include <stdint.h>

namespace pichi::crypto {

extern size_t generateKey(CryptoMethod, ConstBuffer, MutableBuffer);

}  // namespace pichi::crypto

#endif  // PICHI_CRYPTO_KEY_HPP
