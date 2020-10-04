#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/messages.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value toJson(Egress const& evo, Allocator& alloc)
{
  auto egress_ = json::Value{};
  egress_.SetObject();
  egress_.AddMember(egress::TYPE, toJson(evo.type_, alloc), alloc);
  if (evo.type_ != AdapterType::DIRECT && evo.type_ != AdapterType::REJECT) {
    assertTrue(evo.host_.has_value(), PichiError::MISC);
    assertFalse(evo.host_->empty(), PichiError::MISC);
    assertTrue(evo.port_.has_value(), PichiError::MISC);
    assertFalse(*evo.port_ == 0_u16, PichiError::MISC);
    egress_.AddMember(egress::HOST, toJson(*evo.host_, alloc), alloc);
    egress_.AddMember(egress::PORT, json::Value{*evo.port_}, alloc);
  }
  switch (evo.type_) {
  case AdapterType::SS:
    assertTrue(evo.method_.has_value(), PichiError::MISC);
    assertTrue(evo.password_.has_value(), PichiError::MISC);
    assertFalse(evo.password_->empty(), PichiError::MISC);
    egress_.AddMember(egress::METHOD, toJson(*evo.method_, alloc), alloc);
    egress_.AddMember(egress::PASSWORD, toJson(*evo.password_, alloc), alloc);
    break;
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    assertTrue(evo.tls_.has_value(), PichiError::MISC);
    egress_.AddMember(egress::TLS, *evo.tls_, alloc);
    if (*evo.tls_) {
      assertTrue(evo.insecure_.has_value(), PichiError::MISC);
      egress_.AddMember(egress::INSECURE, *evo.insecure_, alloc);
      if (!*evo.insecure_ && evo.caFile_.has_value()) {
        assertFalse(evo.caFile_->empty());
        egress_.AddMember(egress::CA_FILE, toJson(*evo.caFile_, alloc), alloc);
      }
    }
    break;
  case AdapterType::TROJAN:
    assertTrue(evo.password_.has_value());
    assertFalse(evo.password_->empty());
    egress_.AddMember(egress::PASSWORD, toJson(*evo.password_, alloc), alloc);
    assertTrue(evo.insecure_.has_value());
    egress_.AddMember(egress::INSECURE, *evo.insecure_, alloc);
    if (!*evo.insecure_ && evo.caFile_.has_value()) {
      assertFalse(evo.caFile_->empty());
      egress_.AddMember(egress::CA_FILE, toJson(*evo.caFile_, alloc), alloc);
    }
    break;
  case AdapterType::REJECT:
    assertTrue(evo.mode_.has_value());
    egress_.AddMember(egress::MODE, toJson(*evo.mode_, alloc), alloc);
    if (*evo.mode_ == DelayMode::FIXED) {
      assertTrue(evo.delay_.has_value());
      assertTrue(*evo.delay_ <= 300_u16);
      egress_.AddMember(egress::DELAY, json::Value{*evo.delay_}, alloc);
    }
    break;
  case AdapterType::DIRECT:
    break;
  default:
    fail(PichiError::MISC);
  }

  return egress_;
}

template <> Egress parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(egress::TYPE), PichiError::BAD_JSON, msg::MISSING_TYPE_FIELD);

  auto evo = Egress{};
  evo.type_ = parse<AdapterType>(v[egress::TYPE]);

  if (evo.type_ == AdapterType::HTTP || evo.type_ == AdapterType::SOCKS5 ||
      evo.type_ == AdapterType::SS || evo.type_ == AdapterType::TROJAN) {
    assertTrue(v.HasMember(egress::HOST), PichiError::BAD_JSON, msg::MISSING_HOST_FIELD);
    assertTrue(v.HasMember(egress::PORT), PichiError::BAD_JSON, msg::MISSING_PORT_FIELD);
    evo.host_ = parse<string>(v[egress::HOST]);
    evo.port_ = parse<uint16_t>(v[egress::PORT]);
  }

  switch (evo.type_) {
  case AdapterType::SS:
    assertTrue(v.HasMember(egress::METHOD), PichiError::BAD_JSON, msg::MISSING_METHOD_FIELD);
    assertTrue(v.HasMember(egress::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
    evo.method_ = parse<CryptoMethod>(v[egress::METHOD]);
    evo.password_ = parse<string>(v[egress::PASSWORD]);
    break;
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    evo.tls_ = v.HasMember(egress::TLS) && parse<bool>(v[egress::TLS]);
    if (*evo.tls_) {
      evo.insecure_ = v.HasMember(egress::INSECURE) && parse<bool>(v[egress::INSECURE]);
      if (!*evo.insecure_ && v.HasMember(egress::CA_FILE))
        evo.caFile_ = parse<string>(v[egress::CA_FILE]);
    }
    if (v.HasMember(egress::CREDENTIAL)) {
      evo.credential_ = parsePair(v[egress::CREDENTIAL], parseNameOrPassword);
    }
    break;
  case AdapterType::TROJAN:
    evo.insecure_ = v.HasMember(egress::INSECURE) && parse<bool>(v[egress::INSECURE]);
    if (!*evo.insecure_ && v.HasMember(egress::CA_FILE))
      evo.caFile_ = parse<string>(v[egress::CA_FILE]);
    assertTrue(v.HasMember(egress::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
    evo.password_ = parse<string>(v[egress::PASSWORD]);
    break;
  case AdapterType::REJECT:
    if (v.HasMember(egress::MODE)) {
      evo.mode_ = parse<DelayMode>(v[egress::MODE]);
      if (evo.mode_ == DelayMode::FIXED) {
        assertTrue(v.HasMember(egress::DELAY), PichiError::BAD_JSON, msg::MISSING_DELAY_FIELD);
        assertTrue(v[egress::DELAY].IsInt(), PichiError::BAD_JSON, msg::INT_TYPE_ERROR);
        auto delay = v[egress::DELAY].GetInt();
        assertTrue(delay >= 0_u16 && delay <= 300_u16, PichiError::BAD_JSON, msg::DL_INVALID);
        evo.delay_ = static_cast<uint16_t>(delay);
      }
    }
    else {
      evo.mode_ = DelayMode::FIXED;
      evo.delay_ = 0_u16;
    }
    break;
  case AdapterType::DIRECT:
    break;
  default:
    fail(PichiError::BAD_JSON, msg::AT_INVALID);
  }

  return evo;
}

bool operator==(Egress const& lhs, Egress const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.host_ == rhs.host_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_ && lhs.mode_ == rhs.mode_ &&
         lhs.delay_ == rhs.delay_ && lhs.tls_ == rhs.tls_ && lhs.insecure_ == rhs.insecure_ &&
         lhs.caFile_ == rhs.caFile_;
}

}  // namespace pichi::vo
