#include <array>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/crypto/base64.hpp>
#include <string>

using namespace std;

namespace pichi::crypto {

static auto const OTECTS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"sv;

static char otect2offset(char otect, PichiError e)
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
    fail(e);
}

string base64Encode(string_view text)
{
  if (text.empty()) return ""s;

  auto padding = (3 - (text.size() % 3)) % 3;
  auto len = (text.size() + padding) / 3 * 4;
  auto base64 = string(len, '\0');
  auto i = 0;
  while (i + 4_sz < len) {
    auto j = i / 4 * 3;
    base64[i] = OTECTS[(text[j] >> 2) & 0x3f];
    base64[i + 1] = OTECTS[((text[j] << 4) + ((text[j + 1] >> 4) & 0x0f)) & 0x3f];
    base64[i + 2] = OTECTS[((text[j + 1] << 2) + ((text[j + 2] >> 6) & 0x03)) & 0x3f];
    base64[i + 3] = OTECTS[text[j + 2] & 0x3f];
    i += 4;
  }
  auto j = i / 4 * 3;
  base64[i] = OTECTS[(text[j] >> 2) & 0x3f];
  switch (padding) {
  case 0:
    base64[i + 1] = OTECTS[((text[j] << 4) + ((text[j + 1] >> 4) & 0x0f)) & 0x3f];
    base64[i + 2] = OTECTS[((text[j + 1] << 2) + ((text[j + 2] >> 6) & 0x03)) & 0x3f];
    base64[i + 3] = OTECTS[text[j + 2] & 0x3f];
    break;
  case 1:
    base64[i + 1] = OTECTS[((text[j] << 4) + ((text[j + 1] >> 4) & 0x0f)) & 0x3f];
    base64[i + 2] = OTECTS[(text[j + 1] << 2) & 0x3c];
    base64[i + 3] = '=';
    break;
  default:
    base64[i + 1] = OTECTS[(text[j] << 4) & 0x30];
    base64[i + 2] = '=';
    base64[i + 3] = '=';
    break;
  }
  return base64;
}

string base64Decode(string_view base64, PichiError e)
{
  if (base64.empty()) return {};
  assertTrue(base64.size() % 4 == 0, e);
  auto padding = base64.find_last_not_of('=');
  padding = padding == string_view::npos ? 0_sz : base64.size() - padding - 1_sz;
  assertTrue(padding < 3, e);

  auto text = string(base64.size() / 4 * 3 - padding, '\0');
  auto i = 0;
  while (i + 3_sz < text.size()) {
    auto j = i / 3 * 4;
    text[i] = (otect2offset(base64[j], e) << 2) + (otect2offset(base64[j + 1], e) >> 4);
    text[i + 1] = (otect2offset(base64[j + 1], e) << 4) + (otect2offset(base64[j + 2], e) >> 2);
    text[i + 2] = (otect2offset(base64[j + 2], e) << 6) + otect2offset(base64[j + 3], e);
    i += 3;
  }
  auto j = i / 3 * 4;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif  // __GNUC__

  switch (padding) {
  case 0:
    text[i + 2] = (otect2offset(base64[j + 2], e) << 6) + otect2offset(base64[j + 3], e);
    // Don't break here
  case 1:
    text[i + 1] = (otect2offset(base64[j + 1], e) << 4) + (otect2offset(base64[j + 2], e) >> 2);
    // Don't break here
  default:
    text[i] = (otect2offset(base64[j], e) << 2) + (otect2offset(base64[j + 1], e) >> 4);
    break;
  }

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

  return text;
}

}  // namespace pichi::crypto
