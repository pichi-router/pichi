#define BOOST_TEST_MODULE pichi vo_ingress test

#include "utils.hpp"
#include "vo.hpp"
#include <boost/mpl/set.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

#ifdef _MSC_VER
#ifdef OPTIONAL
#undef OPTIONAL
#endif  // OPTIONAL
#endif  // _MSC_VER

using namespace std;
using namespace rapidjson;
namespace mpl = boost::mpl;

namespace pichi::unit_test {

BOOST_AUTO_TEST_SUITE(VO_INGRESS)

using namespace pichi::vo;

enum class Present { MANDATORY, OPTIONAL, UNUSED };

template <AdapterType type> struct AdapterTrait {
};

template <> struct AdapterTrait<AdapterType::HTTP> {
  static const AdapterType type_ = AdapterType::HTTP;
  static const Present credential_ = Present::OPTIONAL;
  static const Present option_ = Present::UNUSED;
  static const Present tls_ = Present::OPTIONAL;
  static const Present websocket_ = Present::UNUSED;
  using Credential = UpIngressCredential;
};

template <> struct AdapterTrait<AdapterType::SOCKS5> {
  static const AdapterType type_ = AdapterType::SOCKS5;
  static const Present credential_ = Present::OPTIONAL;
  static const Present option_ = Present::UNUSED;
  static const Present tls_ = Present::OPTIONAL;
  static const Present websocket_ = Present::UNUSED;
  using Credential = UpIngressCredential;
  using Tls = TlsIngressOption;
};

template <> struct AdapterTrait<AdapterType::TUNNEL> {
  static const AdapterType type_ = AdapterType::TUNNEL;
  static const Present credential_ = Present::UNUSED;
  static const Present option_ = Present::MANDATORY;
  static const Present tls_ = Present::UNUSED;
  static const Present websocket_ = Present::UNUSED;
  using Option = TunnelOption;
};

template <> struct AdapterTrait<AdapterType::SS> {
  static const AdapterType type_ = AdapterType::SS;
  static const Present credential_ = Present::UNUSED;
  static const Present option_ = Present::MANDATORY;
  static const Present tls_ = Present::UNUSED;
  static const Present websocket_ = Present::UNUSED;
  using Option = ShadowsocksOption;
};

template <> struct AdapterTrait<AdapterType::TROJAN> {
  static const AdapterType type_ = AdapterType::TROJAN;
  static const Present credential_ = Present::MANDATORY;
  static const Present option_ = Present::MANDATORY;
  static const Present tls_ = Present::MANDATORY;
  static const Present websocket_ = Present::OPTIONAL;
  using Credential = TrojanIngressCredential;
  using Option = TrojanOption;
};

template <> struct AdapterTrait<AdapterType::VMESS> {
  static const AdapterType type_ = AdapterType::VMESS;
  static const Present credential_ = Present::MANDATORY;
  static const Present option_ = Present::UNUSED;
  static const Present tls_ = Present::OPTIONAL;
  static const Present websocket_ = Present::OPTIONAL;
  using Credential = VMessIngressCredential;
};

using AllAdapterTraits =
    mpl::set<AdapterTrait<AdapterType::HTTP>, AdapterTrait<AdapterType::SOCKS5>,
             AdapterTrait<AdapterType::TUNNEL>, AdapterTrait<AdapterType::SS>,
             AdapterTrait<AdapterType::TROJAN>, AdapterTrait<AdapterType::VMESS>>;

template <AdapterType type> Value defaultIngressJson()
{
  using Trait = AdapterTrait<type>;
  auto ingress = Value{kObjectType};
  ingress.AddMember(ingress::TYPE, toJson(type, alloc), alloc);
  ingress.AddMember(ingress::BIND, Value{kArrayType}, alloc);
  ingress[ingress::BIND].PushBack(toJson(DEFAULT_ENDPOINT, alloc), alloc);
  if constexpr (Trait::credential_ == Present::MANDATORY)
    ingress.AddMember(ingress::CREDENTIALS, defaultCredentialJson<typename Trait::Credential>(),
                      alloc);
  if constexpr (Trait::option_ == Present::MANDATORY)
    ingress.AddMember(ingress::OPTION, defaultOptionJson<typename Trait::Option>(), alloc);
  if constexpr (Trait::tls_ == Present::MANDATORY)
    ingress.AddMember(ingress::TLS, defaultOptionJson<TlsIngressOption>(), alloc);
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    ingress.AddMember(ingress::WEBSOCKET, defaultOptionJson<WebsocketOption>(), alloc);

  return ingress;
}

template <AdapterType type> Ingress defaultIngress()
{
  using Trait = AdapterTrait<type>;
  auto ingress = Ingress{};
  ingress.type_ = type;
  ingress.bind_.push_back(DEFAULT_ENDPOINT);
  if constexpr (Trait::credential_ == Present::MANDATORY)
    ingress.credential_ = defaultCredential<typename Trait::Credential>();
  if constexpr (Trait::option_ == Present::MANDATORY)
    ingress.opt_ = defaultOption<typename Trait::Option>();
  if constexpr (Trait::tls_ == Present::MANDATORY) ingress.tls_ = defaultOption<TlsIngressOption>();
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    ingress.websocket_ = defaultOption<WebsocketOption>();
  return ingress;
}

template <AdapterType type, typename Key> void verifyMandatoryField(Key&& key)
{
  auto json = defaultIngressJson<type>();
  json.RemoveMember(key);
  BOOST_CHECK_EXCEPTION(parse<Ingress>(json), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Default_Ones, Trait, AllAdapterTraits)
{
  BOOST_CHECK(defaultIngress<Trait::type_>() == parse<Ingress>(defaultIngressJson<Trait::type_>()));
}

BOOST_AUTO_TEST_CASE(parse_Invalid_Json_Type)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kArrayType}) {
    BOOST_CHECK_EXCEPTION(parse<Ingress>(Value{t}), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Basic_Mandatory_Fields, Trait, AllAdapterTraits)
{
  auto noType = defaultIngressJson<Trait::type_>();
  noType.RemoveMember(ingress::TYPE);
  BOOST_CHECK_EXCEPTION(parse<Ingress>(noType), Exception, verifyException<PichiError::BAD_JSON>);

  auto noBind = defaultIngressJson<Trait::type_>();
  noBind.RemoveMember(ingress::BIND);
  BOOST_CHECK_EXCEPTION(parse<Ingress>(noBind), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Invalid_Types, Trait, AllAdapterTraits)
{
  for (auto t : {AdapterType::DIRECT, AdapterType::REJECT}) {
    auto invalid = defaultIngressJson<Trait::type_>();
    invalid[ingress::TYPE] = toJson(t, alloc);
    BOOST_CHECK_EXCEPTION(parse<Ingress>(invalid), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Invalid_Bind_Type, Trait, AllAdapterTraits)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kObjectType}) {
    auto json = defaultIngressJson<Trait::type_>();
    json[ingress::BIND] = Value{t};
    BOOST_CHECK_EXCEPTION(parse<Ingress>(json), Exception, verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Empty_Bind, Trait, AllAdapterTraits)
{
  auto json = defaultIngressJson<Trait::type_>();
  json[ingress::BIND].Clear();
  BOOST_CHECK_EXCEPTION(parse<Ingress>(json), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Mandatory_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::credential_ == Present::MANDATORY)
    verifyMandatoryField<Trait::type_>(ingress::CREDENTIALS);
  if constexpr (Trait::option_ == Present::MANDATORY)
    verifyMandatoryField<Trait::type_>(ingress::OPTION);
  if constexpr (Trait::tls_ == Present::MANDATORY) verifyMandatoryField<Trait::type_>(ingress::TLS);
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    verifyMandatoryField<Trait::type_>(ingress::WEBSOCKET);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Optional_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::credential_ == Present::OPTIONAL) {
    using Credential = typename Trait::Credential;
    auto ingress = defaultIngress<Trait::type_>();
    ingress.credential_ = defaultCredential<Credential>();
    BOOST_CHECK(parse<Ingress>(defaultIngressJson<Trait::type_>().AddMember(
                    ingress::CREDENTIALS, defaultCredentialJson<Credential>(), alloc)) == ingress);
  }
  if constexpr (Trait::option_ == Present::OPTIONAL) {
    using Option = typename Trait::Option;
    auto ingress = defaultIngress<Trait::type_>();
    ingress.opt_ = defaultOption<Option>();
    BOOST_CHECK(parse<Ingress>(defaultIngressJson<Trait::type_>().AddMember(
                    ingress::OPTION, defaultOptionJson<Option>(), alloc)) == ingress);
  }
  if constexpr (Trait::tls_ == Present::OPTIONAL) {
    auto ingress = defaultIngress<Trait::type_>();
    ingress.tls_ = defaultOption<TlsIngressOption>();
    BOOST_CHECK(parse<Ingress>(defaultIngressJson<Trait::type_>().AddMember(
                    ingress::TLS, defaultOptionJson<TlsIngressOption>(), alloc)) == ingress);
  }
  if constexpr (Trait::websocket_ == Present::OPTIONAL) {
    auto ingress = defaultIngress<Trait::type_>();
    ingress.websocket_ = defaultOption<WebsocketOption>();
    BOOST_CHECK(parse<Ingress>(defaultIngressJson<Trait::type_>().AddMember(
                    ingress::WEBSOCKET, defaultOptionJson<WebsocketOption>(), alloc)) == ingress);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Unused_Fields, Trait, AllAdapterTraits)
{
  auto json = defaultIngressJson<Trait::type_>();
  if constexpr (Trait::credential_ == Present::UNUSED)
    json.AddMember(ingress::CREDENTIALS, Value{kObjectType}, alloc);
  if constexpr (Trait::option_ == Present::UNUSED)
    json.AddMember(ingress::OPTION, Value{kObjectType}, alloc);
  if constexpr (Trait::tls_ == Present::UNUSED)
    json.AddMember(ingress::TLS, defaultOptionJson<TlsIngressOption>(), alloc);
  if constexpr (Trait::websocket_ == Present::UNUSED)
    json.AddMember(ingress::WEBSOCKET, defaultOptionJson<WebsocketOption>(), alloc);
  BOOST_CHECK(parse<Ingress>(json) == defaultIngress<Trait::type_>());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Default_Ones, Trait, AllAdapterTraits)
{
  BOOST_CHECK(defaultIngressJson<Trait::type_>() == toJson(defaultIngress<Trait::type_>(), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Invalid_Adapter_Type)
{
  for (auto type : {AdapterType::DIRECT, AdapterType::REJECT}) {
    auto ingress = Ingress{};
    ingress.type_ = type;
    ingress.bind_.push_back(DEFAULT_ENDPOINT);
    BOOST_CHECK_EXCEPTION(toJson(ingress, alloc), Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Empty_Bind, Trait, AllAdapterTraits)
{
  auto ingress = defaultIngress<Trait::type_>();
  ingress.bind_.clear();
  BOOST_CHECK_EXCEPTION(toJson(ingress, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Mandatory_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::credential_ == Present::MANDATORY) {
    auto ingress = defaultIngress<Trait::type_>();
    ingress.credential_.reset();
    BOOST_CHECK_EXCEPTION(toJson(ingress, alloc), Exception, verifyException<PichiError::MISC>);
  }
  if constexpr (Trait::option_ == Present::MANDATORY) {
    auto ingress = defaultIngress<Trait::type_>();
    ingress.opt_.reset();
    BOOST_CHECK_EXCEPTION(toJson(ingress, alloc), Exception, verifyException<PichiError::MISC>);
  }
  if constexpr (Trait::tls_ == Present::MANDATORY) {
    auto ingress = defaultIngress<Trait::type_>();
    ingress.tls_.reset();
    BOOST_CHECK_EXCEPTION(toJson(ingress, alloc), Exception, verifyException<PichiError::MISC>);
  }
  if constexpr (Trait::websocket_ == Present::MANDATORY) {
    auto ingress = defaultIngress<Trait::type_>();
    ingress.websocket_.reset();
    BOOST_CHECK_EXCEPTION(toJson(ingress, alloc), Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Optional_Fields, Trait, AllAdapterTraits)
{
  auto ingress = defaultIngress<Trait::type_>();
  auto json = defaultIngressJson<Trait::type_>();

  if constexpr (Trait::credential_ == Present::OPTIONAL) {
    ingress.credential_ = defaultCredential<typename Trait::Credential>();
    json.AddMember(ingress::CREDENTIALS, defaultCredentialJson<typename Trait::Credential>(),
                   alloc);
  }
  if constexpr (Trait::option_ == Present::OPTIONAL) {
    ingress.opt_ = defaultOption<typename Trait::Option>();
    json.AddMember(ingress::OPTION, defaultOptionJson<typename Trait::Option>(), alloc);
  }
  if constexpr (Trait::tls_ == Present::OPTIONAL) {
    ingress.tls_ = defaultOption<TlsIngressOption>();
    json.AddMember(ingress::TLS, defaultOptionJson<TlsIngressOption>(), alloc);
  }
  if constexpr (Trait::websocket_ == Present::OPTIONAL) {
    ingress.websocket_ = defaultOption<WebsocketOption>();
    json.AddMember(ingress::WEBSOCKET, defaultOptionJson<WebsocketOption>(), alloc);
  }

  BOOST_CHECK(toJson(ingress, alloc) == json);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Unused_Fields, Trait, AllAdapterTraits)
{
  auto ingress = defaultIngress<Trait::type_>();
  if constexpr (Trait::credential_ == Present::UNUSED)
    ingress.credential_ = defaultCredential<UpIngressCredential>();
  if constexpr (Trait::option_ == Present::UNUSED) ingress.opt_ = defaultOption<TunnelOption>();
  if constexpr (Trait::tls_ == Present::UNUSED) ingress.tls_ = defaultOption<TlsIngressOption>();
  if constexpr (Trait::websocket_ == Present::UNUSED)
    ingress.websocket_ = defaultOption<WebsocketOption>();
  BOOST_CHECK(toJson(ingress, alloc) == defaultIngressJson<Trait::type_>());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
