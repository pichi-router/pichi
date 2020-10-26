#ifndef PICHI_TEST_VO_HPP
#define PICHI_TEST_VO_HPP

#include "utils.hpp"
#include <boost/mpl/set.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/vo/credential.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/options.hpp>
#include <rapidjson/document.h>
#include <type_traits>

namespace pichi::unit_test {

using AllCredentials = boost::mpl::set<vo::UpIngressCredential, vo::TrojanIngressCredential,
                                       vo::VMessIngressCredential, vo::UpEgressCredential,
                                       vo::TrojanEgressCredential, vo::VMessEgressCredential>;

using IngressCredentials = boost::mpl::set<vo::UpIngressCredential, vo::TrojanIngressCredential,
                                           vo::VMessIngressCredential>;

using EgressCredentials =
    boost::mpl::set<vo::UpEgressCredential, vo::TrojanEgressCredential, vo::VMessEgressCredential>;

template <typename Credential> Credential defaultCredential();
template <typename Credential> rapidjson::Value defaultCredentialJson();

using AllOptions =
    boost::mpl::set<vo::ShadowsocksOption, vo::TunnelOption, vo::RejectOption, vo::TrojanOption,
                    vo::TlsIngressOption, vo::TlsEgressOption, vo::WebsocketOption>;

template <typename Key, typename Set> using HasKeyT = typename boost::mpl::has_key<Set, Key>::type;
template <typename Key, typename Set> inline constexpr bool HasKey = HasKeyT<Key, Set>::value;

template <typename Option> rapidjson::Value defaultOptionJson();
template <typename Option> Option defaultOption();

extern rapidjson::Document::AllocatorType& alloc;
extern Endpoint const DEFAULT_ENDPOINT;

extern vo::Egress defaultEgressVO(AdapterType);
extern rapidjson::Value defaultEgressJson(AdapterType);

template <typename Modifier> rapidjson::Value createJsonObject(Modifier&& modify)
{
  static_assert(std::is_invocable_v<Modifier, rapidjson::Value&>);
  auto ret = rapidjson::Value{rapidjson::kObjectType};
  modify(ret);
  return ret;
}

template <typename Modifier> rapidjson::Value createJsonArray(Modifier&& modify)
{
  auto ret = rapidjson::Value{rapidjson::kArrayType};
  ret.PushBack(createJsonObject(std::forward<Modifier>(modify)), alloc);
  return ret;
}

}  // namespace pichi::unit_test

#endif  // PICHI_TEST_VO_HPP
