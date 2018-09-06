#include "utils.hpp"
#include <sodium.h>
#include <string.h>

using namespace std;

namespace pichi {

vector<uint8_t> str2vec(string_view s) { return {cbegin(s), cend(s)}; }

vector<uint8_t> hex2bin(string_view hex)
{
  // auto hlen = strlen(hex);
  auto v = vector<uint8_t>(hex.size() / 2, 0);
  sodium_hex2bin(v.data(), v.size(), hex.data(), hex.size(), nullptr, nullptr, nullptr);
  return v;
}

} // namespace pichi
