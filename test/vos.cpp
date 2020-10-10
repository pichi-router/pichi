#define BOOST_TEST_MODULE pichi vos test

#include "utils.hpp"
#include "vo.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <unordered_map>

using namespace std;
using namespace rapidjson;

namespace pichi::unit_test {

using pichi::vo::parse;
using pichi::vo::toJson;

template <typename String> auto createValue(String const& str)
{
  return Value{str.data(), static_cast<SizeType>(str.size())};
}

template <typename Map, typename Executor, typename Checker>
void verifyPairs(Map const& map, Executor&& execute, Checker&& check)
{
  for_each(cbegin(map), cend(map),
           [execute = forward<Executor>(execute), check = forward<Checker>(check)](auto&& pair) {
             auto&& [key, expect] = pair;
             check(expect, execute(key));
           });
}

template <typename T> void verifyParsing(unordered_map<string_view, T> const& map)
{
  verifyPairs(
      map, [](auto str) { return parse<T>(createValue(str)); },
      [](auto&& lhs, auto&& rhs) { BOOST_CHECK(lhs == rhs); });
}

template <typename T> void verifyToJson(unordered_map<T, string_view> const& map)
{
  verifyPairs(
      map, [](auto t) { return toJson(t, alloc); },
      [](auto&& expect, auto&& fact) {
        BOOST_CHECK(fact.IsString());
        BOOST_CHECK_EQUAL(expect, fact.GetString());
      });
}

BOOST_AUTO_TEST_SUITE(VOS)

BOOST_AUTO_TEST_CASE(parse_AdapterType)
{
  verifyParsing<AdapterType>({{vo::type::DIRECT, AdapterType::DIRECT},
                              {vo::type::REJECT, AdapterType::REJECT},
                              {vo::type::SOCKS5, AdapterType::SOCKS5},
                              {vo::type::HTTP, AdapterType::HTTP},
                              {vo::type::SS, AdapterType::SS},
                              {vo::type::TUNNEL, AdapterType::TUNNEL}});
}

BOOST_AUTO_TEST_CASE(toJson_AdapterType)
{
  verifyToJson<AdapterType>({{AdapterType::DIRECT, vo::type::DIRECT},
                             {AdapterType::REJECT, vo::type::REJECT},
                             {AdapterType::SOCKS5, vo::type::SOCKS5},
                             {AdapterType::HTTP, vo::type::HTTP},
                             {AdapterType::SS, vo::type::SS},
                             {AdapterType::TUNNEL, vo::type::TUNNEL}});
}

BOOST_AUTO_TEST_CASE(parse_CryptoMethod)
{
  verifyParsing<CryptoMethod>(
      {{vo::method::RC4_MD5, CryptoMethod::RC4_MD5},
       {vo::method::BF_CFB, CryptoMethod::BF_CFB},
       {vo::method::AES_128_CTR, CryptoMethod::AES_128_CTR},
       {vo::method::AES_192_CTR, CryptoMethod::AES_192_CTR},
       {vo::method::AES_256_CTR, CryptoMethod::AES_256_CTR},
       {vo::method::AES_128_CFB, CryptoMethod::AES_128_CFB},
       {vo::method::AES_192_CFB, CryptoMethod::AES_192_CFB},
       {vo::method::AES_256_CFB, CryptoMethod::AES_256_CFB},
       {vo::method::CAMELLIA_128_CFB, CryptoMethod::CAMELLIA_128_CFB},
       {vo::method::CAMELLIA_192_CFB, CryptoMethod::CAMELLIA_192_CFB},
       {vo::method::CAMELLIA_256_CFB, CryptoMethod::CAMELLIA_256_CFB},
       {vo::method::CHACHA20, CryptoMethod::CHACHA20},
       {vo::method::SALSA20, CryptoMethod::SALSA20},
       {vo::method::CHACHA20_IETF, CryptoMethod::CHACHA20_IETF},
       {vo::method::AES_128_GCM, CryptoMethod::AES_128_GCM},
       {vo::method::AES_192_GCM, CryptoMethod::AES_192_GCM},
       {vo::method::AES_256_GCM, CryptoMethod::AES_256_GCM},
       {vo::method::CHACHA20_IETF_POLY1305, CryptoMethod::CHACHA20_IETF_POLY1305},
       {vo::method::XCHACHA20_IETF_POLY1305, CryptoMethod::XCHACHA20_IETF_POLY1305}});
}

BOOST_AUTO_TEST_CASE(toJson_CryptoMethod)
{
  verifyToJson<CryptoMethod>({
      {CryptoMethod::RC4_MD5, vo::method::RC4_MD5},
      {CryptoMethod::BF_CFB, vo::method::BF_CFB},
      {CryptoMethod::AES_128_CTR, vo::method::AES_128_CTR},
      {CryptoMethod::AES_192_CTR, vo::method::AES_192_CTR},
      {CryptoMethod::AES_256_CTR, vo::method::AES_256_CTR},
      {CryptoMethod::AES_128_CFB, vo::method::AES_128_CFB},
      {CryptoMethod::AES_192_CFB, vo::method::AES_192_CFB},
      {CryptoMethod::AES_256_CFB, vo::method::AES_256_CFB},
      {CryptoMethod::CAMELLIA_128_CFB, vo::method::CAMELLIA_128_CFB},
      {CryptoMethod::CAMELLIA_192_CFB, vo::method::CAMELLIA_192_CFB},
      {CryptoMethod::CAMELLIA_256_CFB, vo::method::CAMELLIA_256_CFB},
      {CryptoMethod::CHACHA20, vo::method::CHACHA20},
      {CryptoMethod::SALSA20, vo::method::SALSA20},
      {CryptoMethod::CHACHA20_IETF, vo::method::CHACHA20_IETF},
      {CryptoMethod::AES_128_GCM, vo::method::AES_128_GCM},
      {CryptoMethod::AES_192_GCM, vo::method::AES_192_GCM},
      {CryptoMethod::AES_256_GCM, vo::method::AES_256_GCM},
      {CryptoMethod::CHACHA20_IETF_POLY1305, vo::method::CHACHA20_IETF_POLY1305},
      {CryptoMethod::XCHACHA20_IETF_POLY1305, vo::method::XCHACHA20_IETF_POLY1305},
  });
}

BOOST_AUTO_TEST_CASE(parse_Balance)
{
  verifyParsing<BalanceType>({{vo::balance::RANDOM, BalanceType::RANDOM},
                              {vo::balance::ROUND_ROBIN, BalanceType::ROUND_ROBIN},
                              {vo::balance::LEAST_CONN, BalanceType::LEAST_CONN}});
}

BOOST_AUTO_TEST_CASE(toJson_Balance)
{
  verifyToJson<BalanceType>({{BalanceType::RANDOM, vo::balance::RANDOM},
                             {BalanceType::ROUND_ROBIN, vo::balance::ROUND_ROBIN},
                             {BalanceType::LEAST_CONN, vo::balance::LEAST_CONN}});
}

BOOST_AUTO_TEST_CASE(parse_DelayMode)
{
  verifyParsing<DelayMode>(
      {{vo::delay::FIXED, DelayMode::FIXED}, {vo::delay::RANDOM, DelayMode::RANDOM}});
}

BOOST_AUTO_TEST_CASE(toJson_DelayMode)
{
  verifyToJson<DelayMode>(
      {{DelayMode::FIXED, vo::delay::FIXED}, {DelayMode::RANDOM, vo::delay::RANDOM}});
}

BOOST_AUTO_TEST_CASE(parse_VMessSecurity)
{
  verifyParsing<VMessSecurity>(
      {{vo::security::AUTO, VMessSecurity::AUTO},
       {vo::security::NONE, VMessSecurity::NONE},
       {vo::security::CHACHA20_IETF_POLY1305, VMessSecurity::CHACHA20_IETF_POLY1305},
       {vo::security::AES_128_GCM, VMessSecurity::AES_128_GCM}});
}

BOOST_AUTO_TEST_CASE(toJson_VMessSecurity)
{
  verifyToJson<VMessSecurity>(
      {{VMessSecurity::AUTO, vo::security::AUTO},
       {VMessSecurity::NONE, vo::security::NONE},
       {VMessSecurity::CHACHA20_IETF_POLY1305, vo::security::CHACHA20_IETF_POLY1305},
       {VMessSecurity::AES_128_GCM, vo::security::AES_128_GCM}});
}

BOOST_AUTO_TEST_CASE(parse_Uint16_Incorrect_Type)
{
  for (auto&& v : {Value{0.0}, Value{kStringType}, Value{kNullType}, Value{kTrueType},
                   Value{kFalseType}, Value{kObjectType}, Value{kArrayType}}) {
    BOOST_CHECK_EXCEPTION(vo::parse<uint16_t>(v), Exception, verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Uint16_Out_Of_Range)
{
  BOOST_CHECK_EXCEPTION(vo::parse<uint16_t>(Value{-1}), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(
      vo::parse<uint16_t>(Value{static_cast<uint32_t>(numeric_limits<uint16_t>::max()) + 1}),
      Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Uint16_All_Numbers)
{
  for (auto port = 0; port <= numeric_limits<uint16_t>::max(); ++port) {
    BOOST_CHECK_EQUAL(port, vo::parse<uint16_t>(Value{port}));
  }
}

BOOST_AUTO_TEST_CASE(parseNameOrPassword_Length_Out_Of_Range)
{
  auto s = string(256, 'a');
  BOOST_CHECK_EXCEPTION(vo::parseNameOrPassword(createValue(s)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parseNameOrPassword_All_Length)
{
  auto s = ""s;
  for (auto len = 1; len < 256; ++len) {
    s.push_back('a');
    BOOST_CHECK_EQUAL(s, vo::parseNameOrPassword(createValue(s)));
  }
}

BOOST_AUTO_TEST_CASE(parseDestinantions_Incorrect_Type)
{
  for (auto&& v : {Value{0}, Value{0.0}, Value{kStringType}, Value{kNullType}, Value{kTrueType},
                   Value{kFalseType}, Value{kArrayType}}) {
    BOOST_CHECK_EXCEPTION(vo::parseDestinantions(v), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parseDestinantions_Empty_Object)
{
  BOOST_CHECK_EXCEPTION(vo::parseDestinantions(Value{kObjectType}), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parseDestinations_Incorrect_Port_Type)
{
  for (auto&& v : {kStringType, kNullType, kTrueType, kFalseType, kObjectType, kArrayType}) {
    auto json = Value{kObjectType};
    json.AddMember(ph, Value{v}, alloc);
    BOOST_CHECK_EXCEPTION(vo::parseDestinantions(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parseDestinations_Correct_One)
{
  auto json = Value{kObjectType};
  json.AddMember("localhost", 16, alloc);
  auto dests = vo::parseDestinantions(json);

  BOOST_CHECK_EQUAL(1, dests.size());
  BOOST_CHECK(makeEndpoint("localhost", 16) == dests.front());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
