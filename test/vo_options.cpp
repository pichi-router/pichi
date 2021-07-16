#define BOOST_TEST_MODULE pichi vo_options test

#include "utils.hpp"
#include "vo.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/options.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

using namespace std;
using namespace rapidjson;

namespace pichi::unit_test {

BOOST_AUTO_TEST_SUITE(VO_OPTIONS)

using namespace pichi::vo;

template <typename Option, typename Key> Value generateJsonWithout(Key&& key)
{
  auto json = defaultOptionJson<Option>();
  json.RemoveMember(forward<Key>(key));
  return json;
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Option_Normal, Option, AllOptions)
{
  BOOST_CHECK(parse<Option>(defaultOptionJson<Option>()) == defaultOption<Option>());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Option_Normal, Option, AllOptions)
{
  BOOST_CHECK(toJson(defaultOption<Option>(), alloc) == defaultOptionJson<Option>());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Option_Invalid_Type, Option, AllOptions)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kArrayType}) {
    BOOST_CHECK_EXCEPTION(parse<Option>(Value{t}), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_ShadowsocksOption_Mandatory_Fields)
{
  BOOST_CHECK_EXCEPTION(
      parse<ShadowsocksOption>(generateJsonWithout<ShadowsocksOption>(option::METHOD)), Exception,
      verifyException<PichiError::BAD_JSON>);

  BOOST_CHECK_EXCEPTION(
      parse<ShadowsocksOption>(generateJsonWithout<ShadowsocksOption>(option::PASSWORD)), Exception,
      verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_TunnelOption_Mandatory_Fields)
{
  BOOST_CHECK_EXCEPTION(
      parse<TunnelOption>(generateJsonWithout<TunnelOption>(option::DESTINATIONS)), Exception,
      verifyException<PichiError::BAD_JSON>);

  BOOST_CHECK_EXCEPTION(parse<TunnelOption>(generateJsonWithout<TunnelOption>(option::BALANCE)),
                        Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_TunnelOption_Invalid_Type_Of_Destinations)
{
  auto invalid = defaultOptionJson<TunnelOption>();
  invalid[option::DESTINATIONS].SetObject();
  BOOST_CHECK_EXCEPTION(parse<TunnelOption>(invalid), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_TunnelOption_Empty_Destinations)
{
  auto empty = defaultOptionJson<TunnelOption>();
  empty[option::DESTINATIONS].Clear();
  BOOST_CHECK_EXCEPTION(parse<TunnelOption>(empty), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(toJson_TunnelOption_Empty_Destinations)
{
  auto emptyDest = defaultOption<TunnelOption>();
  emptyDest.destinations_.clear();
  BOOST_CHECK_EXCEPTION(toJson(emptyDest, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_RejectOption_Delay_Out_Of_Range)
{
  auto out = defaultOptionJson<RejectOption>();
  out[option::DELAY] = 301_u16;
  BOOST_CHECK_EXCEPTION(parse<RejectOption>(out), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_RejectOption_Default_Fields)
{
  auto json = Value{kObjectType};
  json.AddMember(option::MODE, toJson(DelayMode::FIXED, alloc), alloc);
  auto def = parse<RejectOption>(json);
  BOOST_CHECK(def.mode_ == DelayMode::FIXED);
  BOOST_CHECK(def.delay_.has_value());
  BOOST_CHECK_EQUAL(*def.delay_, 0_u16);
}

BOOST_AUTO_TEST_CASE(parse_RejectOption_Ignoring_Delay)
{
  auto json = Value{kObjectType};
  json.AddMember(option::MODE, toJson(DelayMode::RANDOM, alloc), alloc);
  json.AddMember(option::DELAY, 0_u16, alloc);
  auto hasDelay = parse<RejectOption>(json);
  BOOST_CHECK(hasDelay.mode_ == DelayMode::RANDOM);
  BOOST_CHECK(!hasDelay.delay_.has_value());
}

BOOST_AUTO_TEST_CASE(toJson_RejectOption_Ignoring_Delay_When_Random)
{
  auto ignore = toJson(RejectOption{DelayMode::RANDOM, {0_u16}}, alloc);
  BOOST_CHECK(ignore.HasMember(option::MODE));
  BOOST_CHECK(parse<DelayMode>(ignore[option::MODE]) == DelayMode::RANDOM);
  BOOST_CHECK(!ignore.HasMember(option::DELAY));
}

BOOST_AUTO_TEST_CASE(toJson_RejectOption_Mandatory_Delay_When_Fixed)
{
  BOOST_CHECK_EXCEPTION(toJson(RejectOption{DelayMode::FIXED, {}}, alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_RejectOption_Delay_Out_Of_Range)
{
  BOOST_CHECK_EXCEPTION(toJson(RejectOption{DelayMode::FIXED, 301_u16}, alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_TrojanOption_Mandatory_Fields)
{
  BOOST_CHECK_EXCEPTION(parse<TrojanOption>(generateJsonWithout<TrojanOption>(option::REMOTE)),
                        Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_TlsIngressOption_Mandatory_Fields)
{
  BOOST_CHECK_EXCEPTION(
      parse<TlsIngressOption>(generateJsonWithout<TlsIngressOption>(tls::CERT_FILE)), Exception,
      verifyException<PichiError::BAD_JSON>);

  BOOST_CHECK_EXCEPTION(
      parse<TlsIngressOption>(generateJsonWithout<TlsIngressOption>(tls::KEY_FILE)), Exception,
      verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_TlsEgressOption_Default_Values)
{
  auto def = parse<TlsEgressOption>(Value{kObjectType});
  BOOST_CHECK(!def.insecure_);
  BOOST_CHECK(!def.caFile_.has_value());
  BOOST_CHECK(!def.serverName_.has_value());
  BOOST_CHECK(!def.sni_.has_value());
}

BOOST_AUTO_TEST_CASE(parse_TlsEgressOption_Ignoring_Fields_When_Insecure)
{
  auto json = defaultOptionJson<TlsEgressOption>();
  json[tls::INSECURE] = true;
  auto ignore = parse<TlsEgressOption>(json);
  BOOST_CHECK(ignore.insecure_);
  BOOST_CHECK(!ignore.serverName_.has_value());
  BOOST_CHECK(!ignore.caFile_.has_value());
  BOOST_CHECK(ignore.sni_.has_value());
  BOOST_CHECK_EQUAL(*ignore.sni_, ph);
}

BOOST_AUTO_TEST_CASE(parse_TlsEgressOption_Optional_Fields)
{
  auto noCaFile = parse<TlsEgressOption>(generateJsonWithout<TlsEgressOption>(tls::CA_FILE));
  BOOST_CHECK(!noCaFile.insecure_);
  BOOST_CHECK(!noCaFile.caFile_.has_value());

  auto noServerName =
      parse<TlsEgressOption>(generateJsonWithout<TlsEgressOption>(tls::SERVER_NAME));
  BOOST_CHECK(!noServerName.insecure_);
  BOOST_CHECK(!noServerName.serverName_.has_value());

  auto noSni = parse<TlsEgressOption>(generateJsonWithout<TlsEgressOption>(tls::SNI));
  BOOST_CHECK(!noSni.sni_.has_value());
}

BOOST_AUTO_TEST_CASE(toJson_TlsEgressOption_Optional_Fields)
{
  for (auto insecure : {true, false}) {
    auto json = toJson(TlsEgressOption{insecure, {}, {}, {}}, alloc);
    BOOST_CHECK(json.IsObject());
    BOOST_CHECK(!json.HasMember(tls::SNI));
    BOOST_CHECK(!json.HasMember(tls::CA_FILE));
    BOOST_CHECK(!json.HasMember(tls::SERVER_NAME));
  }
}

BOOST_AUTO_TEST_CASE(parse_WebsocketOption_Mandatory_Fields)
{
  BOOST_CHECK_EXCEPTION(
      parse<WebsocketOption>(generateJsonWithout<WebsocketOption>(websocket::PATH)), Exception,
      verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_WebsocketOption_Optional_Fields)
{
  auto noHost = parse<WebsocketOption>(generateJsonWithout<WebsocketOption>(websocket::HOST));
  BOOST_CHECK(!noHost.host_.has_value());
}

BOOST_AUTO_TEST_CASE(toJson_WebsocketOption_Optional_Fields)
{
  auto option = defaultOption<WebsocketOption>();
  option.host_.reset();
  auto json = toJson(option, alloc);
  BOOST_CHECK(json.IsObject());
  BOOST_CHECK(!json.HasMember(websocket::HOST));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
