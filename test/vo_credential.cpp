#define BOOST_TEST_MODULE pichi vo_credential test

#include "utils.hpp"
#include <boost/mpl/set.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/credential.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <unordered_map>

using namespace std;
using namespace rapidjson;
namespace mpl = boost::mpl;

namespace pichi::unit_test {

BOOST_AUTO_TEST_SUITE(VO_CREDENTIAL)

using namespace pichi::vo;

using AllCredentials =
    mpl::set<up::IngressCredential, trojan::IngressCredential, vmess::IngressCredential,
             up::EgressCredential, trojan::EgressCredential, vmess::EgressCredential>;

using IngressCredentials =
    mpl::set<up::IngressCredential, trojan::IngressCredential, vmess::IngressCredential>;

using EgressCredentials =
    mpl::set<up::EgressCredential, trojan::EgressCredential, vmess::EgressCredential>;

template <typename Credential> Credential defaultCredential()
{
  if constexpr (is_same_v<Credential, up::IngressCredential>) {
    return up::IngressCredential{{{ph, ph}}};
  }
  else if constexpr (is_same_v<Credential, trojan::IngressCredential>) {
    return trojan::IngressCredential{{ph}};
  }
  else if constexpr (is_same_v<Credential, vmess::IngressCredential>) {
    return vmess::IngressCredential{{{ph, 0_u16}}};
  }
  else if constexpr (is_same_v<Credential, up::EgressCredential>) {
    return up::EgressCredential{make_pair(ph, ph)};
  }
  else if constexpr (is_same_v<Credential, trojan::EgressCredential>) {
    return trojan::EgressCredential{ph};
  }
  else if constexpr (is_same_v<Credential, vmess::EgressCredential>) {
    return vmess::EgressCredential{ph, 0_u16, VMessSecurity::AUTO};
  }
  else {
    BOOST_FAIL("Unacceptable adapter type");
    return 0;
  }
}

template <typename Modifier> Value createJsonObject(Modifier&& modify)
{
  static_assert(is_invocable_v<Modifier, Value&>);
  auto ret = Value{kObjectType};
  modify(ret);
  return ret;
}

template <typename Modifier> Value createJsonArray(Modifier&& modify)
{
  auto ret = Value{kArrayType};
  ret.PushBack(createJsonObject(std::forward<Modifier>(modify)), alloc);
  return ret;
}

template <typename Credential> Value defaultCredentialJson()
{
  if constexpr (is_same_v<Credential, up::IngressCredential>) {
    return createJsonArray([](auto&& item) {
      item.AddMember(credential::USERNAME, ph, alloc).AddMember(credential::PASSWORD, ph, alloc);
    });
  }
  else if constexpr (is_same_v<Credential, trojan::IngressCredential>) {
    return createJsonArray([](auto&& item) { item.AddMember(credential::PASSWORD, ph, alloc); });
  }
  else if constexpr (is_same_v<Credential, vmess::IngressCredential>) {
    return createJsonArray([](auto&& item) {
      item.AddMember(credential::UUID, ph, alloc).AddMember(credential::ALTER_ID, Value{0}, alloc);
    });
  }
  else if constexpr (is_same_v<Credential, up::EgressCredential>) {
    return createJsonObject([](auto&& item) {
      item.AddMember(credential::USERNAME, ph, alloc).AddMember(credential::PASSWORD, ph, alloc);
    });
  }
  else if constexpr (is_same_v<Credential, trojan::EgressCredential>) {
    return createJsonObject([](auto&& item) { item.AddMember(credential::PASSWORD, ph, alloc); });
  }
  else if constexpr (is_same_v<Credential, vmess::EgressCredential>) {
    return createJsonObject([](auto&& item) {
      item.AddMember(credential::UUID, ph, alloc)
          .AddMember(credential::ALTER_ID, Value{0}, alloc)
          .AddMember(credential::SECURITY, toJson(VMessSecurity::AUTO, alloc), alloc);
    });
  }
  else {
    BOOST_FAIL("Unacceptable adapter type");
    return {};
  }
}

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
    BOOST_CHECK_EXCEPTION(parse<Credential>(Value{t}), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Empty_Credential, Credential,
                              IngressCredentials)
{
  BOOST_CHECK_EXCEPTION(parse<Credential>(Value{kArrayType}), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Invalid_Type_Of_Items, Credential,
                              IngressCredentials)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kArrayType}) {
    auto v = Value{kArrayType};
    v.PushBack(Value{t}, alloc);
    BOOST_CHECK_EXCEPTION(parse<Credential>(v), Exception, verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Mandatory_Fields, Credential,
                              IngressCredentials)
{
  auto keys = vector<string>{};
  if constexpr (is_same_v<Credential, up::IngressCredential>) {
    keys.push_back(credential::USERNAME);
    keys.push_back(credential::PASSWORD);
  }
  else if constexpr (is_same_v<Credential, trojan::IngressCredential>) {
    keys.push_back(credential::PASSWORD);
  }
  else if constexpr (is_same_v<Credential, vmess::IngressCredential>) {
    keys.push_back(credential::UUID);
  }
  for (auto&& key : keys) {
    auto json = defaultCredentialJson<Credential>();
    json[0].RemoveMember(key.c_str());
    BOOST_CHECK_EXCEPTION(parse<Credential>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Duplicated_Key, Credential,
                              IngressCredentials)
{
  auto modify = function<void(Value&)>{};
  if constexpr (is_same_v<Credential, up::IngressCredential>) {
    modify = [](auto&& item) {
      item.AddMember(credential::USERNAME, ph, alloc).AddMember(credential::PASSWORD, ph, alloc);
    };
  }
  else if constexpr (is_same_v<Credential, trojan::IngressCredential>) {
    modify = [](auto&& item) { item.AddMember(credential::PASSWORD, ph, alloc); };
  }
  else if constexpr (is_same_v<Credential, vmess::IngressCredential>) {
    modify = [](auto&& item) { item.AddMember(credential::UUID, ph, alloc); };
  }
  auto json = defaultCredentialJson<Credential>();
  json.PushBack(createJsonObject(modify), alloc);
  BOOST_CHECK_EXCEPTION(parse<Credential>(json), Exception,
                        verifyException<PichiError::SEMANTIC_ERROR>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_IngressCredential_Empty_Key, Credential, IngressCredentials)
{
  auto key = ""s;
  if constexpr (is_same_v<Credential, up::IngressCredential>) {
    key = credential::USERNAME;
  }
  else if constexpr (is_same_v<Credential, trojan::IngressCredential>) {
    key = credential::PASSWORD;
  }
  else if constexpr (is_same_v<Credential, vmess::IngressCredential>) {
    key = credential::UUID;
  }
  auto empty = defaultCredentialJson<Credential>();
  empty[0][key.c_str()] = "";
  BOOST_CHECK_EXCEPTION(parse<Credential>(empty), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_IngressCredential_Empty_Credential, Credential,
                              IngressCredentials)
{
  BOOST_CHECK_EXCEPTION(toJson(Credential{}, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_EgressCredential_Invalid_Type, Credential, EgressCredentials)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kArrayType}) {
    BOOST_CHECK_EXCEPTION(parse<Credential>(Value{t}), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_EgressCredential_Mandatory_Fields, Credential,
                              EgressCredentials)
{
  auto keys = vector<string>{};
  if constexpr (is_same_v<Credential, up::EgressCredential>) {
    keys.push_back(credential::USERNAME);
    keys.push_back(credential::PASSWORD);
  }
  else if constexpr (is_same_v<Credential, trojan::EgressCredential>) {
    keys.push_back(credential::PASSWORD);
  }
  else if constexpr (is_same_v<Credential, vmess::EgressCredential>) {
    keys.push_back(credential::UUID);
  }
  for (auto&& key : keys) {
    auto json = defaultCredentialJson<Credential>();
    json.RemoveMember(key.c_str());
    BOOST_CHECK_EXCEPTION(parse<Credential>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_vmess_EgressCredential_Default_Fields)
{
  using Credential = vmess::EgressCredential;

  auto alterId = defaultCredentialJson<Credential>();
  alterId.RemoveMember(credential::ALTER_ID);
  BOOST_CHECK(parse<Credential>(alterId) == defaultCredential<Credential>());

  auto security = defaultCredentialJson<Credential>();
  security.RemoveMember(credential::SECURITY);
  BOOST_CHECK(parse<Credential>(security) == defaultCredential<Credential>());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
