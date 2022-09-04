#define BOOST_TEST_MODULE pichi vo_credential test

#include "utils.hpp"
#include "vo.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/vo/credential.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <unordered_map>

using namespace std;
using namespace rapidjson;

namespace pichi::unit_test {

BOOST_AUTO_TEST_SUITE(VO_CREDENTIAL)

using namespace pichi::vo;

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Credential_Normal_Case, Credential, AllCredentials)
{
  BOOST_CHECK(parse<Credential>(defaultCredentialJson<Credential>()) ==
              defaultCredential<Credential>());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Credential_Normal_Case, Credential, AllCredentials)
{
  BOOST_CHECK(toJson(defaultCredential<Credential>(), alloc) ==
              defaultCredentialJson<Credential>());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Invalid_Type, Credential, IngressCredentials)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kObjectType}) {
    BOOST_CHECK_EXCEPTION(parse<Credential>(Value{t}), SystemError,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Empty_Credential, Credential,
                              IngressCredentials)
{
  BOOST_CHECK_EXCEPTION(parse<Credential>(Value{kArrayType}), SystemError,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Invalid_Type_Of_Items, Credential,
                              IngressCredentials)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kArrayType}) {
    auto v = Value{kArrayType};
    v.PushBack(Value{t}, alloc);
    BOOST_CHECK_EXCEPTION(parse<Credential>(v), SystemError, verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Mandatory_Fields, Credential,
                              IngressCredentials)
{
  auto keys = vector<string>{};
  if constexpr (is_same_v<Credential, UpIngressCredential>) {
    keys.push_back(credential::USERNAME);
    keys.push_back(credential::PASSWORD);
  }
  else if constexpr (is_same_v<Credential, TrojanIngressCredential>) {
    keys.push_back(credential::PASSWORD);
  }
  else if constexpr (is_same_v<Credential, VMessIngressCredential>) {
    keys.push_back(credential::UUID);
  }
  for (auto&& key : keys) {
    auto json = defaultCredentialJson<Credential>();
    json[0].RemoveMember(key.c_str());
    BOOST_CHECK_EXCEPTION(parse<Credential>(json), SystemError,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Duplicated_Key, Credential,
                              IngressCredentials)
{
  auto modify = function<void(Value&)>{};
  if constexpr (is_same_v<Credential, UpIngressCredential>) {
    modify = [](auto&& item) {
      item.AddMember(credential::USERNAME, ph, alloc).AddMember(credential::PASSWORD, ph, alloc);
    };
  }
  else if constexpr (is_same_v<Credential, TrojanIngressCredential>) {
    modify = [](auto&& item) { item.AddMember(credential::PASSWORD, ph, alloc); };
  }
  else if constexpr (is_same_v<Credential, VMessIngressCredential>) {
    modify = [](auto&& item) { item.AddMember(credential::UUID, ph, alloc); };
  }
  auto json = defaultCredentialJson<Credential>();
  json.PushBack(createJsonObject(modify), alloc);
  BOOST_CHECK_EXCEPTION(parse<Credential>(json), SystemError,
                        verifyException<PichiError::SEMANTIC_ERROR>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Empty_Key, Credential, IngressCredentials)
{
  auto key = ""s;
  if constexpr (is_same_v<Credential, UpIngressCredential>) {
    key = credential::USERNAME;
  }
  else if constexpr (is_same_v<Credential, TrojanIngressCredential>) {
    key = credential::PASSWORD;
  }
  else if constexpr (is_same_v<Credential, VMessIngressCredential>) {
    key = credential::UUID;
  }
  auto empty = defaultCredentialJson<Credential>();
  empty[0][key.c_str()] = "";
  BOOST_CHECK_EXCEPTION(parse<Credential>(empty), SystemError,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_IngressCredential_Empty_Credential, Credential,
                              IngressCredentials)
{
  BOOST_CHECK_EXCEPTION(toJson(Credential{}, alloc), SystemError,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_EgressCredential_Invalid_Type, Credential, EgressCredentials)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kArrayType}) {
    BOOST_CHECK_EXCEPTION(parse<Credential>(Value{t}), SystemError,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_EgressCredential_Mandatory_Fields, Credential,
                              EgressCredentials)
{
  auto keys = vector<string>{};
  if constexpr (is_same_v<Credential, UpEgressCredential>) {
    keys.push_back(credential::USERNAME);
    keys.push_back(credential::PASSWORD);
  }
  else if constexpr (is_same_v<Credential, TrojanEgressCredential>) {
    keys.push_back(credential::PASSWORD);
  }
  else if constexpr (is_same_v<Credential, VMessEgressCredential>) {
    keys.push_back(credential::UUID);
  }
  for (auto&& key : keys) {
    auto json = defaultCredentialJson<Credential>();
    json.RemoveMember(key.c_str());
    BOOST_CHECK_EXCEPTION(parse<Credential>(json), SystemError,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_vmess_EgressCredential_Default_Fields)
{
  using Credential = VMessEgressCredential;

  auto alterId = defaultCredentialJson<Credential>();
  alterId.RemoveMember(credential::ALTER_ID);
  BOOST_CHECK(parse<Credential>(alterId) == defaultCredential<Credential>());

  auto security = defaultCredentialJson<Credential>();
  security.RemoveMember(credential::SECURITY);
  BOOST_CHECK(parse<Credential>(security) == defaultCredential<Credential>());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
