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

#ifdef _WIN32
#ifdef OPTIONAL
#undef OPTIONAL
#endif  // OPTIONAL
#endif  // _WIN32

namespace json = rapidjson;
namespace mpl  = boost::mpl;

namespace pichi::unit_test {

BOOST_AUTO_TEST_SUITE(VO_INGRESS)

enum class Present { MANDATORY, OPTIONAL, UNUSED };

template <AdapterType type> struct AdapterTrait {};

template <> struct AdapterTrait<AdapterType::HTTP> {
  static const AdapterType type_       = AdapterType::HTTP;
  static const Present     credential_ = Present::OPTIONAL;
  static const Present     option_     = Present::UNUSED;
  static const Present     tls_        = Present::OPTIONAL;
  static const Present     websocket_  = Present::UNUSED;
  using Credential                     = vo::UpIngressCredential;
};

template <> struct AdapterTrait<AdapterType::SOCKS5> {
  static const AdapterType type_       = AdapterType::SOCKS5;
  static const Present     credential_ = Present::OPTIONAL;
  static const Present     option_     = Present::UNUSED;
  static const Present     tls_        = Present::OPTIONAL;
  static const Present     websocket_  = Present::UNUSED;
  using Credential                     = vo::UpIngressCredential;
};

template <> struct AdapterTrait<AdapterType::TUNNEL> {
  static const AdapterType type_       = AdapterType::TUNNEL;
  static const Present     credential_ = Present::UNUSED;
  static const Present     option_     = Present::MANDATORY;
  static const Present     tls_        = Present::UNUSED;
  static const Present     websocket_  = Present::UNUSED;
  using Option                         = vo::TunnelOption;
};

template <> struct AdapterTrait<AdapterType::SS> {
  static const AdapterType type_       = AdapterType::SS;
  static const Present     credential_ = Present::UNUSED;
  static const Present     option_     = Present::MANDATORY;
  static const Present     tls_        = Present::UNUSED;
  static const Present     websocket_  = Present::UNUSED;
  using Option                         = vo::ShadowsocksOption;
};

template <> struct AdapterTrait<AdapterType::TROJAN> {
  static const AdapterType type_       = AdapterType::TROJAN;
  static const Present     credential_ = Present::MANDATORY;
  static const Present     option_     = Present::MANDATORY;
  static const Present     tls_        = Present::MANDATORY;
  static const Present     websocket_  = Present::OPTIONAL;
  using Credential                     = vo::TrojanIngressCredential;
  using Option                         = vo::TrojanOption;
};

template <> struct AdapterTrait<AdapterType::TRANSPARENT> {
  static const AdapterType type_       = AdapterType::TRANSPARENT;
  static const Present     credential_ = Present::UNUSED;
  static const Present     option_     = Present::UNUSED;
  static const Present     tls_        = Present::UNUSED;
  static const Present     websocket_  = Present::UNUSED;
};

template <> struct AdapterTrait<AdapterType::DUAL> {
  static const AdapterType type_       = AdapterType::DUAL;
  static const Present     credential_ = Present::OPTIONAL;
  static const Present     option_     = Present::UNUSED;
  static const Present     tls_        = Present::OPTIONAL;
  static const Present     websocket_  = Present::UNUSED;
  using Credential                     = vo::UpIngressCredential;
};

using AllAdapterTraits = mpl::set<
    AdapterTrait<AdapterType::HTTP>, AdapterTrait<AdapterType::SOCKS5>,
    AdapterTrait<AdapterType::TUNNEL>, AdapterTrait<AdapterType::SS>,
    AdapterTrait<AdapterType::TROJAN>, AdapterTrait<AdapterType::TRANSPARENT>,
    AdapterTrait<AdapterType::DUAL>>;

template <AdapterType type> json::Value default_json()
{
  using Trait  = AdapterTrait<type>;
  auto ingress = json::Value{json::kObjectType};
  ingress.AddMember(vo::ingress::TYPE, vo::toJson(type, alloc), alloc);
  ingress.AddMember(vo::ingress::BIND, json::Value{json::kArrayType}, alloc);
  ingress[vo::ingress::BIND].PushBack(vo::toJson(DEFAULT_ENDPOINT, alloc), alloc);
  if constexpr (Trait::credential_ == Present::MANDATORY)
    ingress.AddMember(
        vo::ingress::CREDENTIALS,
        defaultCredentialJson<typename Trait::Credential>(),
        alloc
    );
  if constexpr (Trait::option_ == Present::MANDATORY)
    ingress.AddMember(vo::ingress::OPTION, defaultOptionJson<typename Trait::Option>(), alloc);
  if constexpr (Trait::tls_ == Present::MANDATORY)
    ingress.AddMember(vo::ingress::TLS, defaultOptionJson<vo::TlsIngressOption>(), alloc);
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    ingress.AddMember(vo::ingress::WEBSOCKET, defaultOptionJson<vo::WebsocketOption>(), alloc);

  return ingress;
}

template <AdapterType type> vo::Ingress default_ingress()
{
  using Trait   = AdapterTrait<type>;
  auto ingress  = vo::Ingress{};
  ingress.type_ = type;
  ingress.bind_.push_back(DEFAULT_ENDPOINT);
  if constexpr (Trait::credential_ == Present::MANDATORY)
    ingress.credential_ = defaultCredential<typename Trait::Credential>();
  if constexpr (Trait::option_ == Present::MANDATORY)
    ingress.opt_ = defaultOption<typename Trait::Option>();
  if constexpr (Trait::tls_ == Present::MANDATORY)
    ingress.tls_ = defaultOption<vo::TlsIngressOption>();
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    ingress.websocket_ = defaultOption<vo::WebsocketOption>();
  return ingress;
}

template <AdapterType type, typename Key> void verify_mandatory_field(Key&& key)
{
  auto json = default_json<type>();
  json.RemoveMember(key);
  BOOST_CHECK_EXCEPTION(
      vo::parse<vo::Ingress>(json),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Default_Ones, Trait, AllAdapterTraits)
{
  BOOST_CHECK(
      default_ingress<Trait::type_>() == vo::parse<vo::Ingress>(default_json<Trait::type_>())
  );
}

BOOST_AUTO_TEST_CASE(parse_Invalid_Json_Type)
{
  for (auto t :
       {json::kNumberType,
        json::kNullType,
        json::kStringType,
        json::kTrueType,
        json::kFalseType,
        json::kArrayType}) {
    BOOST_CHECK_EXCEPTION(
        vo::parse<vo::Ingress>(json::Value{t}),
        SystemError,
        verify_exception<PichiError::BAD_JSON>
    );
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Basic_Mandatory_Fields, Trait, AllAdapterTraits)
{
  auto noType = default_json<Trait::type_>();
  noType.RemoveMember(vo::ingress::TYPE);
  BOOST_CHECK_EXCEPTION(
      vo::parse<vo::Ingress>(noType),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );

  auto noBind = default_json<Trait::type_>();
  noBind.RemoveMember(vo::ingress::BIND);
  BOOST_CHECK_EXCEPTION(
      vo::parse<vo::Ingress>(noBind),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Invalid_Types, Trait, AllAdapterTraits)
{
  for (auto t : {AdapterType::DIRECT, AdapterType::REJECT}) {
    auto invalid               = default_json<Trait::type_>();
    invalid[vo::ingress::TYPE] = vo::toJson(t, alloc);
    BOOST_CHECK_EXCEPTION(
        vo::parse<vo::Ingress>(invalid),
        SystemError,
        verify_exception<PichiError::BAD_JSON>
    );
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Invalid_Bind_Type, Trait, AllAdapterTraits)
{
  for (auto t :
       {json::kNumberType,
        json::kNullType,
        json::kStringType,
        json::kTrueType,
        json::kFalseType,
        json::kObjectType}) {
    auto json               = default_json<Trait::type_>();
    json[vo::ingress::BIND] = json::Value{t};
    BOOST_CHECK_EXCEPTION(
        vo::parse<vo::Ingress>(json),
        SystemError,
        verify_exception<PichiError::BAD_JSON>
    );
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Empty_Bind, Trait, AllAdapterTraits)
{
  auto json = default_json<Trait::type_>();
  json[vo::ingress::BIND].Clear();
  BOOST_CHECK_EXCEPTION(
      vo::parse<vo::Ingress>(json),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Mandatory_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::credential_ == Present::MANDATORY)
    verify_mandatory_field<Trait::type_>(vo::ingress::CREDENTIALS);
  if constexpr (Trait::option_ == Present::MANDATORY)
    verify_mandatory_field<Trait::type_>(vo::ingress::OPTION);
  if constexpr (Trait::tls_ == Present::MANDATORY)
    verify_mandatory_field<Trait::type_>(vo::ingress::TLS);
  if constexpr (Trait::websocket_ == Present::MANDATORY)
    verify_mandatory_field<Trait::type_>(vo::ingress::WEBSOCKET);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Optional_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::credential_ == Present::OPTIONAL) {
    using Credential    = typename Trait::Credential;
    auto ingress        = default_ingress<Trait::type_>();
    ingress.credential_ = defaultCredential<Credential>();
    BOOST_CHECK(
        vo::parse<vo::Ingress>(default_json<Trait::type_>().AddMember(
            vo::ingress::CREDENTIALS,
            defaultCredentialJson<Credential>(),
            alloc
        )) == ingress
    );
  }
  if constexpr (Trait::option_ == Present::OPTIONAL) {
    using Option = typename Trait::Option;
    auto ingress = default_ingress<Trait::type_>();
    ingress.opt_ = defaultOption<Option>();
    BOOST_CHECK(
        vo::parse<vo::Ingress>(default_json<Trait::type_>().AddMember(
            vo::ingress::OPTION,
            defaultOptionJson<Option>(),
            alloc
        )) == ingress
    );
  }
  if constexpr (Trait::tls_ == Present::OPTIONAL) {
    auto ingress = default_ingress<Trait::type_>();
    ingress.tls_ = defaultOption<vo::TlsIngressOption>();
    BOOST_CHECK(
        vo::parse<vo::Ingress>(default_json<Trait::type_>().AddMember(
            vo::ingress::TLS,
            defaultOptionJson<vo::TlsIngressOption>(),
            alloc
        )) == ingress
    );
  }
  if constexpr (Trait::websocket_ == Present::OPTIONAL) {
    auto ingress       = default_ingress<Trait::type_>();
    ingress.websocket_ = defaultOption<vo::WebsocketOption>();
    BOOST_CHECK(
        vo::parse<vo::Ingress>(default_json<Trait::type_>().AddMember(
            vo::ingress::WEBSOCKET,
            defaultOptionJson<vo::WebsocketOption>(),
            alloc
        )) == ingress
    );
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parse_Unused_Fields, Trait, AllAdapterTraits)
{
  auto json = default_json<Trait::type_>();
  if constexpr (Trait::credential_ == Present::UNUSED)
    json.AddMember(vo::ingress::CREDENTIALS, json::Value{json::kObjectType}, alloc);
  if constexpr (Trait::option_ == Present::UNUSED)
    json.AddMember(vo::ingress::OPTION, json::Value{json::kObjectType}, alloc);
  if constexpr (Trait::tls_ == Present::UNUSED)
    json.AddMember(vo::ingress::TLS, defaultOptionJson<vo::TlsIngressOption>(), alloc);
  if constexpr (Trait::websocket_ == Present::UNUSED)
    json.AddMember(vo::ingress::WEBSOCKET, defaultOptionJson<vo::WebsocketOption>(), alloc);
  BOOST_CHECK(vo::parse<vo::Ingress>(json) == default_ingress<Trait::type_>());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Default_Ones, Trait, AllAdapterTraits)
{
  BOOST_CHECK(default_json<Trait::type_>() == vo::toJson(default_ingress<Trait::type_>(), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Invalid_Adapter_Type)
{
  for (auto type : {AdapterType::DIRECT, AdapterType::REJECT}) {
    auto ingress  = vo::Ingress{};
    ingress.type_ = type;
    ingress.bind_.push_back(DEFAULT_ENDPOINT);
    BOOST_CHECK_EXCEPTION(
        vo::toJson(ingress, alloc),
        SystemError,
        verify_exception<PichiError::MISC>
    );
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Empty_Bind, Trait, AllAdapterTraits)
{
  auto ingress = default_ingress<Trait::type_>();
  ingress.bind_.clear();
  BOOST_CHECK_EXCEPTION(
      vo::toJson(ingress, alloc),
      SystemError,
      verify_exception<PichiError::MISC>
  );
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Mandatory_Fields, Trait, AllAdapterTraits)
{
  if constexpr (Trait::credential_ == Present::MANDATORY) {
    auto ingress = default_ingress<Trait::type_>();
    ingress.credential_.reset();
    BOOST_CHECK_EXCEPTION(
        vo::toJson(ingress, alloc),
        SystemError,
        verify_exception<PichiError::MISC>
    );
  }
  if constexpr (Trait::option_ == Present::MANDATORY) {
    auto ingress = default_ingress<Trait::type_>();
    ingress.opt_.reset();
    BOOST_CHECK_EXCEPTION(
        vo::toJson(ingress, alloc),
        SystemError,
        verify_exception<PichiError::MISC>
    );
  }
  if constexpr (Trait::tls_ == Present::MANDATORY) {
    auto ingress = default_ingress<Trait::type_>();
    ingress.tls_.reset();
    BOOST_CHECK_EXCEPTION(
        vo::toJson(ingress, alloc),
        SystemError,
        verify_exception<PichiError::MISC>
    );
  }
  if constexpr (Trait::websocket_ == Present::MANDATORY) {
    auto ingress = default_ingress<Trait::type_>();
    ingress.websocket_.reset();
    BOOST_CHECK_EXCEPTION(
        vo::toJson(ingress, alloc),
        SystemError,
        verify_exception<PichiError::MISC>
    );
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Optional_Fields, Trait, AllAdapterTraits)
{
  auto ingress = default_ingress<Trait::type_>();
  auto json    = default_json<Trait::type_>();

  if constexpr (Trait::credential_ == Present::OPTIONAL) {
    ingress.credential_ = defaultCredential<typename Trait::Credential>();
    json.AddMember(
        vo::ingress::CREDENTIALS,
        defaultCredentialJson<typename Trait::Credential>(),
        alloc
    );
  }
  if constexpr (Trait::option_ == Present::OPTIONAL) {
    ingress.opt_ = defaultOption<typename Trait::Option>();
    json.AddMember(vo::ingress::OPTION, defaultOptionJson<typename Trait::Option>(), alloc);
  }
  if constexpr (Trait::tls_ == Present::OPTIONAL) {
    ingress.tls_ = defaultOption<vo::TlsIngressOption>();
    json.AddMember(vo::ingress::TLS, defaultOptionJson<vo::TlsIngressOption>(), alloc);
  }
  if constexpr (Trait::websocket_ == Present::OPTIONAL) {
    ingress.websocket_ = defaultOption<vo::WebsocketOption>();
    json.AddMember(vo::ingress::WEBSOCKET, defaultOptionJson<vo::WebsocketOption>(), alloc);
  }

  BOOST_CHECK(vo::toJson(ingress, alloc) == json);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(toJson_Unused_Fields, Trait, AllAdapterTraits)
{
  auto ingress = default_ingress<Trait::type_>();
  if constexpr (Trait::credential_ == Present::UNUSED)
    ingress.credential_ = defaultCredential<vo::UpIngressCredential>();
  if constexpr (Trait::option_ == Present::UNUSED) ingress.opt_ = defaultOption<vo::TunnelOption>();
  if constexpr (Trait::tls_ == Present::UNUSED)
    ingress.tls_ = defaultOption<vo::TlsIngressOption>();
  if constexpr (Trait::websocket_ == Present::UNUSED)
    ingress.websocket_ = defaultOption<vo::WebsocketOption>();
  BOOST_CHECK(vo::toJson(ingress, alloc) == default_json<Trait::type_>());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
