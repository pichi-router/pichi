#define BOOST_TEST_MODULE pichi base64 test

#include "utils.hpp"
#include <array>
#include <boost/test/unit_test.hpp>
#include <pichi/common.hpp>
#include <pichi/crypto/base64.hpp>
#include <unordered_map>

using namespace std;
using namespace pichi;
using namespace pichi::crypto;

BOOST_AUTO_TEST_SUITE(Base64)

BOOST_AUTO_TEST_CASE(encode_Empty_String_View)
{
  BOOST_CHECK_EQUAL(""sv, base64Encode(string_view{nullptr, 0}));
}

BOOST_AUTO_TEST_CASE(decode_Empty_String_View)
{
  BOOST_CHECK_EQUAL(""sv, base64Decode(string_view{nullptr, 0}, PichiError::MISC));
}

/*
 * According to RFC 4648 10
 * BASE64("") = ""
 * BASE64("f") = "Zg=="
 * BASE64("fo") = "Zm8="
 * BASE64("foo") = "Zm9v"
 * BASE64("foob") = "Zm9vYg=="
 * BASE64("fooba") = "Zm9vYmE="
 * BASE64("foobar") = "Zm9vYmFy"
 */

BOOST_AUTO_TEST_CASE(encode_RFC4648)
{
  auto pairs = unordered_map<string_view, string_view>{{""sv, ""sv},
                                                       {"f"sv, "Zg=="sv},
                                                       {"fo"sv, "Zm8="sv},
                                                       {"foo"sv, "Zm9v"sv},
                                                       {"foob"sv, "Zm9vYg=="sv},
                                                       {"fooba"sv, "Zm9vYmE="sv},
                                                       {"foobar"sv, "Zm9vYmFy"sv}};
  for (auto p : pairs) {
    BOOST_CHECK_EQUAL(p.second, base64Encode(p.first));
  }
}

BOOST_AUTO_TEST_CASE(decode_RFC4648)
{
  auto pairs = unordered_map<string_view, string_view>{{""sv, ""sv},
                                                       {"Zg=="sv, "f"sv},
                                                       {"Zm8="sv, "fo"sv},
                                                       {"Zm9v"sv, "foo"sv},
                                                       {"Zm9vYg=="sv, "foob"sv},
                                                       {"Zm9vYmE="sv, "fooba"sv},
                                                       {"Zm9vYmFy"sv, "foobar"sv}};
  for (auto p : pairs) {
    BOOST_CHECK_EQUAL(p.second, base64Decode(p.first));
  }
}

BOOST_AUTO_TEST_SUITE_END()
