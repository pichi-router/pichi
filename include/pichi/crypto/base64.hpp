#ifndef PICHI_CRYPTO_BASE64_HPP
#define PICHI_CRYPTO_BASE64_HPP

#include <pichi/buffer.hpp>
#include <stdint.h>

namespace pichi::crypto {

extern size_t base64Encode(ConstBuffer<uint8_t>, MutableBuffer<uint8_t>);
extern size_t base64Decode(ConstBuffer<uint8_t>, MutableBuffer<uint8_t>);

} // namespace pichi::crypto

#endif // PICHI_CRYPTO_BASE64_HPP
