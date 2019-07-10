#ifndef PICHI_CRYPTO_BASE64_HPP
#define PICHI_CRYPTO_BASE64_HPP

#include <stdint.h>
#include <string>
#include <string_view>

namespace pichi::crypto {

extern std::string base64Encode(std::string_view);
extern std::string base64Decode(std::string_view);

} // namespace pichi::crypto

#endif // PICHI_CRYPTO_BASE64_HPP
