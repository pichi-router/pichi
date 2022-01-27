#define BOOST_TEST_MODULE pichi vo_egress test

#include "utils.hpp"
#include "vo.hpp"
#include <boost/mpl/set.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/egress.hpp>
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

BOOST_AUTO_TEST_SUITE(VO_EGRESS)

using namespace pichi::vo;

enum class Present { MANDATORY, OPTIONAL, UNUSED };

template <AdapterType type> struct AdapterTrait {
};

template <> struct AdapterTrait<AdapterType::DIRECT> {
  static const auto type_ = AdapterType::DIRECT;
  static const auto server_ = Present::UNUSED;
  static const auto credential_ = Present::UNUSED;
  static const auto option_ = Present::UNUSED;
  static const auto tls_ = Present::UNUSED;
  static const auto websocket_ = Present::UNUSED;
};

template <> struct AdapterTrait<AdapterType::HTTP> {
  static const auto type_ = AdapterType::HTTP;
  static const auto server_ = Present::MANDATORY;
  static const auto credential_ = Present::OPTIONAL;
  static const auto option_ = Present::UNUSED;
  static const auto tls_ = Present::OPTIONAL;
  static const auto websocket_ = Present::UNUSED;
  using Credential = UpEgressCredential;
};

template <> struct AdapterTrait<AdapterType::SOCKS5> {
  static const auto type_ = AdapterType::SOCKS5;
  static const auto server_ = Present::MANDATORY;
  static const auto credential_ = Present::OPTIONAL;
  static const auto option_ = Present::UNUSED;
  static const auto tls_ = Present::OPTIONAL;
  static const auto websocket_ = Present::UNUSED;
  using Credential = UpEgressCredential;
};

template <> struct AdapterTrait<AdapterType::REJECT> {
  static const auto type_ = AdapterType::REJECT;
  static const auto server_ = Present::UNUSED;
  static const auto credential_ = Present::UNUSED;
  static const auto option_ = Present::MANDATORY;
  static const auto tls_ = Present::UNUSED;
  static const auto websocket_ = Present::UNUSED;
  using Option = RejectOption;
};

template <> struct AdapterTrait<AdapterType::SS> {
  static const auto type_ = AdapterType::SS;
  static const auto server_ = Present::MANDATORY;
  static const auto credential_ = Present::UNUSED;
  static const auto option_ = Present::MANDATORY;
  static const auto tls_ = Present::UNUSED;
  static const auto websocket_ = Present::UNUSED;
  using Option = ShadowsocksOption;
};

template <> struct AdapterTrait<AdapterType::TROJAN> {
  static const auto type_ = AdapterType::TROJAN;
  static const auto server_ = Present::MANDATORY;
  static const auto credential_ = Present::MANDATORY;
  static const auto option_ = Present::UNUSED;
  static const auto tls_ = Present::MANDATORY;
  static const auto websocket_ = Present::OPTIONAL;
  using Credential = TrojanEgressCredential;
};

template <> struct AdapterTrait<AdapterType::VMESS> {
  static const auto type_ = AdapterType::VMESS;
  static const auto server_ = Present::MANDATORY;
  static const auto credential_ = Present::MANDATORY;
  static const auto option_ = Present::UNUSED;
  static const auto tls_ = Present::OPTIONAL;
  static const auto websocket_ = Present::OPTIONAL;
  using Credential = VMessEgressCredential;
};

using AllAdapterTraits =
    mpl::set<AdapterTrait<AdapterType::DIRECT>, AdapterTrait<AdapterType::HTTP>,
             AdapterTrait<AdapterType::SOCKS5>, AdapterTrait<AdapterType::REJECT>,
             AdapterTrait<AdapterType::SS>, AdapterTrait<AdapterType::TROJAN>,
             AdapterTrait<AdapterType::VMESS>>;

template <AdapterType type> Value defaultEgressJson()
{
  using Trait = AdapterTrait<type>;
  auto egress = Value{kObjectType};
  egress.AddMember(egress::TYPE, toJson(type, alloc), alloc);

  if constexpr (Trait::server_ == Present::MANDATORY)
    egress.AddMember(egress::SERVER, toJson(DEFAULT_ENDPOINT, alloc), alloc);
  if constexpr (Trait::credential_ == Present::MANDATORY)
    egress.AddMember(egress::CREDENTIAL, defaultCredentialJson<typename Trait::Credential>(),
                     alloc);
  if constexpr (Trait::option_ == Present::MANDATORY)
    egress.AddMember(egress::OPTION, defaultOptionJson<typename Trait::Option>(), alloc);
  if constexpr (Trait::tls_ == Present::MANDATORY)
    egress.AddMember(egress::TLS, defaultOptionJson<TlsEgressOption>(), alloc);
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    egress.AddMember(egress::WEBSOCKET, defaultOptionJson<WebsocketOption>(), alloc);
  return egress;
}

template <AdapterType type> Egress defaultEgress()
{
  using Trait = AdapterTrait<type>;
  auto egress = Egress{};
  egress.type_ = type;
  if constexpr (Trait::server_ == Present::MANDATORY) egress.server_ = DEFAULT_ENDPOINT;
  if constexpr (Trait::credential_ == Present::MANDATORY)
    egress.credential_ = defaultCredential<typename Trait::Credential>();
  if constexpr (Trait::option_ == Present::MANDATORY)
    egress.opt_ = defaultOption<typename Trait::Option>();
  if constexpr (Trait::tls_ == Present::MANDATORY) egress.tls_ = defaultOption<TlsEgressOption>();
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    egress.websocket_ = defaultOption<WebsocketOption>();
  return egress;
}

template <AdapterType type, typename Key> void verifyMandatoryField(Key&& key)
{
  auto json = defaultEgressJson<type>();
  json.RemoveMember(key);
  BOOST_CHECK_EXCEPTION(parse<Egress>(json), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Default_Ones, Trait, AllAdapterTraits)
{
  BOOST_CHECK(defaultEgress<Trait::type_>() == parse<Egress>(defaultEgressJson<Trait::type_>()));
}

BOOST_AUTO_TEST_CASE(parse_Invalid_Json_Type)
{
  for (auto t : {kNumberType, kNullType, kStringType, kTrueType, kFalseType, kArrayType}) {
    BOOST_CHECK_EXCEPTION(parse<Egress>(Value{t}), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Type_Is_Mandatory, Trait, AllAdapterTraits)
{
  auto noType = defaultEgressJson<Trait::type_>();
  noType.RemoveMember(egress::TYPE);
  BOOST_CHECK_EXCEPTION(parse<Egress>(noType), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Invalid_Types, Trait, AllAdapterTraits)
{
  auto invalid = defaultEgressJson<Trait::type_>();
  invalid[egress::TYPE] = toJson(AdapterType::TUNNEL, alloc);
  BOOST_CHECK_EXCEPTION(parse<Egress>(invalid), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Mandatory_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::server_ == Present::MANDATORY)
    verifyMandatoryField<Trait::type_>(egress::SERVER);
  if constexpr (Trait::credential_ == Present::MANDATORY)
    verifyMandatoryField<Trait::type_>(egress::CREDENTIAL);
  if constexpr (Trait::option_ == Present::MANDATORY)
    verifyMandatoryField<Trait::type_>(egress::OPTION);
  if constexpr (Trait::tls_ == Present::MANDATORY) verifyMandatoryField<Trait::type_>(egress::TLS);
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    verifyMandatoryField<Trait::type_>(egress::WEBSOCKET);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Optional_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::credential_ == Present::OPTIONAL) {
    using Credential = typename Trait::Credential;
    auto egress = defaultEgress<Trait::type_>();
    egress.credential_ = defaultCredential<Credential>();
    BOOST_CHECK(parse<Egress>(defaultEgressJson<Trait::type_>().AddMember(
                    egress::CREDENTIAL, defaultCredentialJson<Credential>(), alloc)) == egress);
  }
  if constexpr (Trait::option_ == Present::OPTIONAL) {
    using Option = typename Trait::Option;
    auto egress = defaultEgress<Trait::type_>();
    egress.opt_ = defaultOption<Option>();
    BOOST_CHECK(parse<Egress>(defaultEgressJson<Trait::type_>().AddMember(
                    egress::OPTION, defaultOptionJson<Option>(), alloc)) == egress);
  }
  if constexpr (Trait::tls_ == Present::OPTIONAL) {
    auto egress = defaultEgress<Trait::type_>();
    egress.tls_ = defaultOption<TlsEgressOption>();
    BOOST_CHECK(parse<Egress>(defaultEgressJson<Trait::type_>().AddMember(
                    egress::TLS, defaultOptionJson<TlsEgressOption>(), alloc)) == egress);
  }
  if constexpr (Trait::websocket_ == Present::OPTIONAL) {
    auto egress = defaultEgress<Trait::type_>();
    egress.websocket_ = defaultOption<WebsocketOption>();
    BOOST_CHECK(parse<Egress>(defaultEgressJson<Trait::type_>().AddMember(
                    egress::WEBSOCKET, defaultOptionJson<WebsocketOption>(), alloc)) == egress);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Unused_Fields, Trait, AllAdapterTraits)
{
  auto json = defaultEgressJson<Trait::type_>();
  if constexpr (Trait::server_ == Present::UNUSED)
    json.AddMember(egress::SERVER, toJson(DEFAULT_ENDPOINT, alloc), alloc);
  if constexpr (Trait::credential_ == Present::UNUSED)
    json.AddMember(egress::CREDENTIAL, Value{kObjectType}, alloc);
  if constexpr (Trait::option_ == Present::UNUSED)
    json.AddMember(egress::OPTION, Value{kObjectType}, alloc);
  if constexpr (Trait::tls_ == Present::UNUSED)
    json.AddMember(egress::TLS, defaultOptionJson<TlsEgressOption>(), alloc);
  if constexpr (Trait::websocket_ == Present::UNUSED)
    json.AddMember(egress::WEBSOCKET, defaultOptionJson<WebsocketOption>(), alloc);
  BOOST_CHECK(parse<Egress>(json) == defaultEgress<Trait::type_>());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Default_Ones, Trait, AllAdapterTraits)
{
  BOOST_CHECK(defaultEgressJson<Trait::type_>() == toJson(defaultEgress<Trait::type_>(), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Invalid_Adapter_Type)
{
  auto egress = Egress{};
  egress.type_ = AdapterType::TUNNEL;
  BOOST_CHECK_EXCEPTION(toJson(egress, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Mandatory_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::server_ == Present::MANDATORY) {
    auto egress = defaultEgress<Trait::type_>();
    egress.server_.reset();
    BOOST_CHECK_EXCEPTION(toJson(egress, alloc), Exception, verifyException<PichiError::MISC>);
  }
  if constexpr (Trait::credential_ == Present::MANDATORY) {
    auto egress = defaultEgress<Trait::type_>();
    egress.credential_.reset();
    BOOST_CHECK_EXCEPTION(toJson(egress, alloc), Exception, verifyException<PichiError::MISC>);
  }
  if constexpr (Trait::option_ == Present::MANDATORY) {
    auto egress = defaultEgress<Trait::type_>();
    egress.opt_.reset();
    BOOST_CHECK_EXCEPTION(toJson(egress, alloc), Exception, verifyException<PichiError::MISC>);
  }
  if constexpr (Trait::tls_ == Present::MANDATORY) {
    auto egress = defaultEgress<Trait::type_>();
    egress.tls_.reset();
    BOOST_CHECK_EXCEPTION(toJson(egress, alloc), Exception, verifyException<PichiError::MISC>);
  }
  if constexpr (Trait::websocket_ == Present::MANDATORY) {
    auto egress = defaultEgress<Trait::type_>();
    egress.websocket_.reset();
    BOOST_CHECK_EXCEPTION(toJson(egress, alloc), Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Optional_Fields, Trait, AllAdapterTraits)
{
  auto egress = defaultEgress<Trait::type_>();
  auto json = defaultEgressJson<Trait::type_>();

  if constexpr (Trait::credential_ == Present::OPTIONAL) {
    egress.credential_ = defaultCredential<typename Trait::Credential>();
    json.AddMember(egress::CREDENTIAL, defaultCredentialJson<typename Trait::Credential>(), alloc);
  }
  if constexpr (Trait::option_ == Present::OPTIONAL) {
    egress.opt_ = defaultOption<typename Trait::Option>();
    json.AddMember(egress::OPTION, defaultOptionJson<typename Trait::Option>(), alloc);
  }
  if constexpr (Trait::tls_ == Present::OPTIONAL) {
    egress.tls_ = defaultOption<TlsEgressOption>();
    json.AddMember(egress::TLS, defaultOptionJson<TlsEgressOption>(), alloc);
  }
  if constexpr (Trait::websocket_ == Present::OPTIONAL) {
    egress.websocket_ = defaultOption<WebsocketOption>();
    json.AddMember(egress::WEBSOCKET, defaultOptionJson<WebsocketOption>(), alloc);
  }

  BOOST_CHECK(toJson(egress, alloc) == json);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Unused_Fields, Trait, AllAdapterTraits)
{
  auto egress = defaultEgress<Trait::type_>();
  if constexpr (Trait::server_ == Present::UNUSED) egress.server_ = DEFAULT_ENDPOINT;
  if constexpr (Trait::credential_ == Present::UNUSED)
    egress.credential_ = defaultCredential<UpEgressCredential>();
  if constexpr (Trait::option_ == Present::UNUSED) egress.opt_ = defaultOption<RejectOption>();
  if constexpr (Trait::tls_ == Present::UNUSED) egress.tls_ = defaultOption<TlsEgressOption>();
  if constexpr (Trait::websocket_ == Present::UNUSED)
    egress.websocket_ = defaultOption<WebsocketOption>();
  BOOST_CHECK(toJson(egress, alloc) == defaultEgressJson<Trait::type_>());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
