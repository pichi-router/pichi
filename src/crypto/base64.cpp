#include <array>
#include <pichi/asserts.hpp>
#include <pichi/common.hpp>
#include <pichi/crypto/base64.hpp>
#include <string_view>

using namespace std;

namespace pichi::crypto {

static auto const OTECTS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="sv;

static auto dp4n(uint8_t const* b, int bp) { return b[bp] >> 2; }

static auto dp4n1(uint8_t const* b, int bp, int padding)
{
  auto r = (b[bp] & 0x03) << 4;
  return padding < 2 ? r + ((b[bp + 1] & 0xf0) >> 4) : r;
}

static auto dp4n2(uint8_t const* b, int bp, int padding)
{
  if (padding == 2) return 64;
  auto r = (b[bp + 1] & 0x0f) << 2;
  return padding < 1 ? r + ((b[bp + 2] & 0xc0) >> 6) : r;
}

static auto dp4n3(uint8_t const* b, int bp, int padding)
{
  return padding == 0 ? b[bp + 2] & 0x3f : 64;
}

static int otect2offset(uint8_t otect)
{
  if (otect >= 'A' && otect <= 'Z') {
    return otect - 'A';
  }
  else if (otect >= 'a' && otect <= 'z') {
    return otect - 'a' + 26;
  }
  else if (otect >= '0' && otect <= '9') {
    return otect - '0' + 52;
  }
  else if (otect == '+') {
    return 62;
  }
  else if (otect == '/') {
    return 63;
  }
  else
    fail();
}

size_t base64Encode(ConstBuffer<uint8_t> src, MutableBuffer<uint8_t> dst)
{
  auto len = src.size() / 3 * 4 + (src.size() % 3 == 0 ? 0 : 4);
  assertTrue(dst.size() >= len);
  auto d = dst.data();
  auto b = src.data();
  for (auto i = 0_sz; i < len; i += 4) {
    auto j = i / 4 * 3;
    auto padding = j + 3 > src.size() ? j + 3 - src.size() : 0;
    d[i] = OTECTS[dp4n(b, j)];
    d[i + 1] = OTECTS[dp4n1(b, j, padding)];
    d[i + 2] = OTECTS[dp4n2(b, j, padding)];
    d[i + 3] = OTECTS[dp4n3(b, j, padding)];
  }
  return len;
}

size_t base64Decode(ConstBuffer<uint8_t> src, MutableBuffer<uint8_t> dst)
{
  if (src.size() == 0) return 0;
  assertTrue(src.size() % 4 == 0);
  auto b = src.data();
  auto d = dst.data();
  auto padding = 0_sz;
  if (b[src.size() - 2] == '=') {
    assertTrue(b[src.size() - 1] == '=');
    padding = 2_sz;
  }
  else if (b[src.size() - 1] == '=')
    padding = 1_sz;
  auto len = src.size() / 4 * 3 - padding;
  for (auto i = 0_sz; i < len; ++i) {
    auto j = i / 3 * 4;
    switch (i % 3) {
    case 0:
      d[i] = static_cast<uint8_t>((otect2offset(b[j]) << 2) + (otect2offset(b[j + 1]) >> 4));
      break;
    case 1:
      d[i] = static_cast<uint8_t>(((otect2offset(b[j + 1]) & 0x0f) << 4) +
                                  (otect2offset(b[j + 2]) >> 2));
      break;
    default:
      d[i] = static_cast<uint8_t>(((otect2offset(b[j + 2]) & 0x03) << 6) + otect2offset(b[j + 3]));
      break;
    }
  }
  return len;
}

} // namespace pichi::crypto
