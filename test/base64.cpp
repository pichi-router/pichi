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
  auto expects = array<string_view, 7>{""sv,         "Zg=="sv,     "Zm8="sv,    "Zm9v"sv,
                                       "Zm9vYg=="sv, "Zm9vYmE="sv, "Zm9vYmFy"sv};
  auto b = array<uint8_t, 6>{'f', 'o', 'o', 'b', 'a', 'r'};
  auto d = array<uint8_t, 8>{};
  for (auto i = 0_sz; i < 7; ++i) {
    auto expect = expects[i];
    BOOST_CHECK_EQUAL(expect.size(), base64Encode({b, i}, d));
    BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(d),
                                  cbegin(d) + expect.size());
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
  auto container = array<uint8_t, 6>{};
  for (auto p : pairs) {
    auto expect = p.second;
    BOOST_CHECK_EQUAL(expect.size(), base64Decode(ConstBuffer<uint8_t>{p.first}, container));
    BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(container),
                                  cbegin(container) + expect.size());
  }
}

BOOST_AUTO_TEST_SUITE_END()
