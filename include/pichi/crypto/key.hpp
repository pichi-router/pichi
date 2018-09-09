#ifndef PICHI_CRYPTO_KEY_HPP
#define PICHI_CRYPTO_KEY_HPP

#include <pichi/buffer.hpp>
#include <pichi/crypto/method.hpp>
#include <stdint.h>

namespace pichi::crypto {

extern size_t generateKey(CryptoMethod, ConstBuffer<uint8_t>, MutableBuffer<uint8_t>);

} // namespace pichi::crypto

#endif
