#define BOOST_TEST_MODULE pichi vos test

#include "utils.hpp"
#include "vo.hpp"
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <ranges>
#include <unordered_map>

using namespace std::literals;
namespace json = rapidjson;
namespace rngs = std::ranges;

namespace pichi::unit_test {

template <typename String> auto create_value(String const& str)
{
  return json::Value{str.data(), static_cast<json::SizeType>(str.size())};
}

template <rngs::range Map, typename Executor, typename Checker>
void verify_pairs(Map const& map, Executor&& execute, Checker&& check)
{
  for (auto&& item : map) {
    auto&& [key, expect] = item;
    check(expect, execute(key));
  }
}

template <typename T> void verify_parsing(std::unordered_map<std::string_view, T> const& map)
{
  verify_pairs(
      map,
      [](auto str) { return vo::parse<T>(create_value(str)); },
      [](auto&& lhs, auto&& rhs) { BOOST_CHECK(lhs == rhs); }
  );
}

template <typename T> void verify_toJson(std::unordered_map<T, std::string_view> const& map)
{
  verify_pairs(
      map,
      [](auto t) { return vo::toJson(t, alloc); },
      [](auto&& expect, auto&& fact) {
        BOOST_CHECK(fact.IsString());
        BOOST_CHECK_EQUAL(expect, fact.GetString());
      }
  );
}

BOOST_AUTO_TEST_SUITE(VOS)

BOOST_AUTO_TEST_CASE(parse_AdapterType)
{
  verify_parsing<AdapterType>({
      {vo::type::DIRECT, AdapterType::DIRECT},
      {vo::type::REJECT, AdapterType::REJECT},
      {vo::type::SOCKS5, AdapterType::SOCKS5},
      {  vo::type::HTTP,   AdapterType::HTTP},
      {    vo::type::SS,     AdapterType::SS},
      {vo::type::TUNNEL, AdapterType::TUNNEL},
      {vo::type::TRANSP, AdapterType::TRANSP},
      {  vo::type::DUAL,   AdapterType::DUAL},
  });
}

BOOST_AUTO_TEST_CASE(toJson_AdapterType)
{
  verify_toJson<AdapterType>({
      {AdapterType::DIRECT, vo::type::DIRECT},
      {AdapterType::REJECT, vo::type::REJECT},
      {AdapterType::SOCKS5, vo::type::SOCKS5},
      {  AdapterType::HTTP,   vo::type::HTTP},
      {    AdapterType::SS,     vo::type::SS},
      {AdapterType::TUNNEL, vo::type::TUNNEL},
      {AdapterType::TRANSP, vo::type::TRANSP},
      {  AdapterType::DUAL,   vo::type::DUAL},
  });
}

BOOST_AUTO_TEST_CASE(parse_CryptoMethod)
{
  verify_parsing<CryptoMethod>({
      {            vo::method::AES_128_GCM,             CryptoMethod::AES_128_GCM},
      {            vo::method::AES_192_GCM,             CryptoMethod::AES_192_GCM},
      {            vo::method::AES_256_GCM,             CryptoMethod::AES_256_GCM},
      { vo::method::CHACHA20_IETF_POLY1305,  CryptoMethod::CHACHA20_IETF_POLY1305},
      {vo::method::XCHACHA20_IETF_POLY1305, CryptoMethod::XCHACHA20_IETF_POLY1305}
  });
}

BOOST_AUTO_TEST_CASE(toJson_CryptoMethod)
{
  verify_toJson<CryptoMethod>({
      {            CryptoMethod::AES_128_GCM,             vo::method::AES_128_GCM},
      {            CryptoMethod::AES_192_GCM,             vo::method::AES_192_GCM},
      {            CryptoMethod::AES_256_GCM,             vo::method::AES_256_GCM},
      { CryptoMethod::CHACHA20_IETF_POLY1305,  vo::method::CHACHA20_IETF_POLY1305},
      {CryptoMethod::XCHACHA20_IETF_POLY1305, vo::method::XCHACHA20_IETF_POLY1305},
  });
}

BOOST_AUTO_TEST_CASE(parse_Balance)
{
  verify_parsing<BalanceType>({
      {     vo::balance::RANDOM,      BalanceType::RANDOM},
      {vo::balance::ROUND_ROBIN, BalanceType::ROUND_ROBIN},
      { vo::balance::LEAST_CONN,  BalanceType::LEAST_CONN}
  });
}

BOOST_AUTO_TEST_CASE(toJson_Balance)
{
  verify_toJson<BalanceType>({
      {     BalanceType::RANDOM,      vo::balance::RANDOM},
      {BalanceType::ROUND_ROBIN, vo::balance::ROUND_ROBIN},
      { BalanceType::LEAST_CONN,  vo::balance::LEAST_CONN}
  });
}

BOOST_AUTO_TEST_CASE(parse_DelayMode)
{
  verify_parsing<DelayMode>({
      { vo::delay::FIXED,  DelayMode::FIXED},
      {vo::delay::RANDOM, DelayMode::RANDOM}
  });
}

BOOST_AUTO_TEST_CASE(toJson_DelayMode)
{
  verify_toJson<DelayMode>({
      { DelayMode::FIXED,  vo::delay::FIXED},
      {DelayMode::RANDOM, vo::delay::RANDOM}
  });
}

BOOST_AUTO_TEST_CASE(parse_Uint16_Incorrect_Type)
{
  for (auto&& v :
       {json::Value{0.0},
        json::Value{json::kStringType},
        json::Value{json::kNullType},
        json::Value{json::kTrueType},
        json::Value{json::kFalseType},
        json::Value{json::kObjectType},
        json::Value{json::kArrayType}}) {
    BOOST_CHECK_EXCEPTION(
        vo::parse<uint16_t>(v),
        SystemError,
        verify_exception<PichiError::BAD_JSON>
    );
  }
}

BOOST_AUTO_TEST_CASE(parse_Uint16_Out_Of_Range)
{
  BOOST_CHECK_EXCEPTION(
      vo::parse<uint16_t>(json::Value{-1}),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
  BOOST_CHECK_EXCEPTION(
      vo::parse<uint16_t>(
          json::Value{static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()) + 1}
      ),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
}

BOOST_AUTO_TEST_CASE(parse_Uint16_All_Numbers)
{
  for (auto port = 0; port <= std::numeric_limits<uint16_t>::max(); ++port) {
    BOOST_CHECK_EQUAL(port, vo::parse<uint16_t>(json::Value{port}));
  }
}

BOOST_AUTO_TEST_CASE(parseNameOrPassword_Length_Out_Of_Range)
{
  auto s = std::string(256, 'a');
  BOOST_CHECK_EXCEPTION(
      vo::parseNameOrPassword(create_value(s)),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
}

BOOST_AUTO_TEST_CASE(parseNameOrPassword_All_Length)
{
  auto s = ""s;
  for (auto len = 1; len < 256; ++len) {
    s.push_back('a');
    BOOST_CHECK_EQUAL(s, vo::parseNameOrPassword(create_value(s)));
  }
}

BOOST_AUTO_TEST_CASE(parseDestinantions_Incorrect_Type)
{
  for (auto&& v :
       {json::Value{0},
        json::Value{0.0},
        json::Value{json::kStringType},
        json::Value{json::kNullType},
        json::Value{json::kTrueType},
        json::Value{json::kFalseType},
        json::Value{json::kArrayType}}) {
    BOOST_CHECK_EXCEPTION(
        vo::parseDestinantions(v),
        SystemError,
        verify_exception<PichiError::BAD_JSON>
    );
  }
}

BOOST_AUTO_TEST_CASE(parseDestinantions_Empty_Object)
{
  BOOST_CHECK_EXCEPTION(
      vo::parseDestinantions(json::Value{json::kObjectType}),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
}

BOOST_AUTO_TEST_CASE(parseDestinations_Incorrect_Port_Type)
{
  for (auto&& v :
       {json::kStringType,
        json::kNullType,
        json::kTrueType,
        json::kFalseType,
        json::kObjectType,
        json::kArrayType}) {
    auto json = json::Value{json::kObjectType};
    json.AddMember(ph, json::Value{v}, alloc);
    BOOST_CHECK_EXCEPTION(
        vo::parseDestinantions(json),
        SystemError,
        verify_exception<PichiError::BAD_JSON>
    );
  }
}

BOOST_AUTO_TEST_CASE(parseDestinations_Correct_One)
{
  auto json = json::Value{json::kObjectType};
  json.AddMember("localhost", 16, alloc);
  auto dests = vo::parseDestinantions(json);

  BOOST_CHECK_EQUAL(1_sz, dests.size());
  BOOST_CHECK(makeEndpoint("localhost", 16) == dests.front());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
