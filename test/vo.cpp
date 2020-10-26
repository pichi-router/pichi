#include "vo.hpp"
#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

using namespace std;
using namespace pichi::vo;
using namespace rapidjson;

namespace pichi::unit_test {

static auto doc = Document{};
Document::AllocatorType& alloc = doc.GetAllocator();

Endpoint const DEFAULT_ENDPOINT = makeEndpoint(ph, 0_u16);

template <typename Credential> Credential defaultCredential()
{
  static_assert(HasKey<Credential, AllCredentials>);
  if constexpr (is_same_v<Credential, UpIngressCredential>) {
    return UpIngressCredential{{{ph, ph}}};
  }
  else if constexpr (is_same_v<Credential, TrojanIngressCredential>) {
    return TrojanIngressCredential{{ph}};
  }
  else if constexpr (is_same_v<Credential, VMessIngressCredential>) {
    return VMessIngressCredential{{{ph, 0_u16}}};
  }
  else if constexpr (is_same_v<Credential, UpEgressCredential>) {
    return UpEgressCredential{make_pair(ph, ph)};
  }
  else if constexpr (is_same_v<Credential, TrojanEgressCredential>) {
    return TrojanEgressCredential{ph};
  }
  else if constexpr (is_same_v<Credential, VMessEgressCredential>) {
    return VMessEgressCredential{ph, 0_u16, VMessSecurity::AUTO};
  }
  return {};
}

template UpIngressCredential defaultCredential();
template TrojanIngressCredential defaultCredential();
template VMessIngressCredential defaultCredential();
template UpEgressCredential defaultCredential();
template TrojanEgressCredential defaultCredential();
template VMessEgressCredential defaultCredential();

template <typename Credential> Value defaultCredentialJson()
{
  static_assert(HasKey<Credential, AllCredentials>);
  if constexpr (is_same_v<Credential, UpIngressCredential>) {
    return createJsonArray([](auto&& item) {
      item.AddMember(credential::USERNAME, ph, alloc).AddMember(credential::PASSWORD, ph, alloc);
    });
  }
  else if constexpr (is_same_v<Credential, TrojanIngressCredential>) {
    return createJsonArray([](auto&& item) { item.AddMember(credential::PASSWORD, ph, alloc); });
  }
  else if constexpr (is_same_v<Credential, VMessIngressCredential>) {
    return createJsonArray([](auto&& item) {
      item.AddMember(credential::UUID, ph, alloc).AddMember(credential::ALTER_ID, Value{0}, alloc);
    });
  }
  else if constexpr (is_same_v<Credential, UpEgressCredential>) {
    return createJsonObject([](auto&& item) {
      item.AddMember(credential::USERNAME, ph, alloc).AddMember(credential::PASSWORD, ph, alloc);
    });
  }
  else if constexpr (is_same_v<Credential, TrojanEgressCredential>) {
    return createJsonObject([](auto&& item) { item.AddMember(credential::PASSWORD, ph, alloc); });
  }
  else if constexpr (is_same_v<Credential, VMessEgressCredential>) {
    return createJsonObject([](auto&& item) {
      item.AddMember(credential::UUID, ph, alloc)
          .AddMember(credential::ALTER_ID, Value{0}, alloc)
          .AddMember(credential::SECURITY, toJson(VMessSecurity::AUTO, alloc), alloc);
    });
  }
  return {};
}

template Value defaultCredentialJson<UpIngressCredential>();
template Value defaultCredentialJson<TrojanIngressCredential>();
template Value defaultCredentialJson<VMessIngressCredential>();
template Value defaultCredentialJson<UpEgressCredential>();
template Value defaultCredentialJson<TrojanEgressCredential>();
template Value defaultCredentialJson<VMessEgressCredential>();

template <typename Option> Value defaultOptionJson()
{
  static_assert(HasKey<Option, AllOptions>);
  auto ret = Value{kObjectType};
  if constexpr (is_same_v<Option, ShadowsocksOption>) {
    ret.AddMember(option::METHOD, toJson(CryptoMethod::RC4_MD5, alloc), alloc);
    ret.AddMember(option::PASSWORD, ph, alloc);
  }
  else if constexpr (is_same_v<Option, TunnelOption>) {
    auto destinations = Value{kArrayType};
    destinations.PushBack(toJson(makeEndpoint(ph, 0_u16), alloc), alloc);
    ret.AddMember(option::DESTINATIONS, destinations, alloc);
    ret.AddMember(option::BALANCE, toJson(BalanceType::RANDOM, alloc), alloc);
  }
  else if constexpr (is_same_v<Option, RejectOption>) {
    ret.AddMember(option::MODE, toJson(DelayMode::FIXED, alloc), alloc);
    ret.AddMember(option::DELAY, Value{0_u16}, alloc);
  }
  else if constexpr (is_same_v<Option, TrojanOption>) {
    ret.AddMember(option::REMOTE, toJson(makeEndpoint(ph, 0_u16), alloc), alloc);
  }
  else if constexpr (is_same_v<Option, TlsIngressOption>) {
    ret.AddMember(tls::CERT_FILE, toJson(ph, alloc), alloc);
    ret.AddMember(tls::KEY_FILE, toJson(ph, alloc), alloc);
  }
  else if constexpr (is_same_v<Option, TlsEgressOption>) {
    ret.AddMember(tls::INSECURE, false, alloc);
    ret.AddMember(tls::CA_FILE, ph, alloc);
    ret.AddMember(tls::SERVER_NAME, ph, alloc);
    ret.AddMember(tls::SNI, ph, alloc);
  }
  else if constexpr (is_same_v<Option, WebsocketOption>) {
    ret.AddMember(websocket::PATH, ph, alloc);
    ret.AddMember(websocket::HOST, ph, alloc);
  }
  return ret;
}

template Value defaultOptionJson<ShadowsocksOption>();
template Value defaultOptionJson<TunnelOption>();
template Value defaultOptionJson<RejectOption>();
template Value defaultOptionJson<TrojanOption>();
template Value defaultOptionJson<TlsIngressOption>();
template Value defaultOptionJson<TlsEgressOption>();
template Value defaultOptionJson<WebsocketOption>();

template <typename Option> Option defaultOption()
{
  static_assert(HasKey<Option, AllOptions>);
  if constexpr (is_same_v<Option, ShadowsocksOption>) {
    return {ph, CryptoMethod::RC4_MD5};
  }
  else if constexpr (is_same_v<Option, TunnelOption>) {
    return {{makeEndpoint(ph, 0_u16)}, BalanceType::RANDOM};
  }
  else if constexpr (is_same_v<Option, RejectOption>) {
    return {DelayMode::FIXED, {0_u16}};
  }
  else if constexpr (is_same_v<Option, TrojanOption>) {
    return {makeEndpoint(ph, 0_u16)};
  }
  else if constexpr (is_same_v<Option, TlsIngressOption>) {
    return {ph, ph};
  }
  else if constexpr (is_same_v<Option, TlsEgressOption>) {
    return {false, {ph}, {ph}, {ph}};
  }
  else if constexpr (is_same_v<Option, WebsocketOption>) {
    return {ph, ph};
  }
  return {};
}

template ShadowsocksOption defaultOption<>();
template TunnelOption defaultOption<>();
template RejectOption defaultOption<>();
template TrojanOption defaultOption<>();
template TlsIngressOption defaultOption<>();
template TlsEgressOption defaultOption<>();
template WebsocketOption defaultOption<>();

vo::Egress defaultEgressVO(AdapterType type)
{
  auto vo = vo::Egress{};
  vo.type_ = type;
  switch (type) {
  case AdapterType::DIRECT:
    break;
  case AdapterType::REJECT:
    vo.mode_ = DelayMode::FIXED;
    vo.delay_ = 0_u16;
    break;
  case AdapterType::SS:
    vo.method_ = CryptoMethod::RC4_MD5;
    vo.password_ = ph;
    vo.host_ = ph;
    vo.port_ = 1_u8;
    break;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    vo.host_ = ph;
    vo.port_ = 1_u8;
    vo.tls_ = false;
    break;
  case AdapterType::TROJAN:
    vo.host_ = ph;
    vo.port_ = 1_u8;
    vo.password_ = ph;
    vo.insecure_ = false;
    break;
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return vo;
}

Value defaultEgressJson(AdapterType type)
{
  auto v = Value{};
  v.SetObject();
  if (type != AdapterType::DIRECT && type != AdapterType::REJECT) {
    v.AddMember(vo::egress::HOST, ph, alloc);
    v.AddMember(vo::egress::PORT, 1, alloc);
  }
  switch (type) {
  case AdapterType::DIRECT:
    v.AddMember(vo::egress::TYPE, vo::type::DIRECT, alloc);
    break;
  case AdapterType::REJECT:
    v.AddMember(vo::egress::TYPE, vo::type::REJECT, alloc);
    v.AddMember(vo::egress::MODE, vo::delay::FIXED, alloc);
    v.AddMember(vo::egress::DELAY, 0, alloc);
    break;
  case AdapterType::HTTP:
    v.AddMember(vo::egress::TYPE, vo::type::HTTP, alloc);
    v.AddMember(vo::egress::TLS, false, alloc);
    break;
  case AdapterType::SOCKS5:
    v.AddMember(vo::egress::TYPE, vo::type::SOCKS5, alloc);
    v.AddMember(vo::egress::TLS, false, alloc);
    break;
  case AdapterType::SS:
    v.AddMember(vo::egress::TYPE, vo::type::SS, alloc);
    v.AddMember(vo::egress::METHOD, vo::method::RC4_MD5, alloc);
    v.AddMember(vo::egress::PASSWORD, ph, alloc);
    break;
  case AdapterType::TROJAN:
    v.AddMember("type", "trojan", alloc);
    v.AddMember("password", ph, alloc);
    break;
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return v;
}

}  // namespace pichi::unit_test
