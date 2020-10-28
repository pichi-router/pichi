#include <numeric>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/messages.hpp>
#include <pichi/vo/options.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

using namespace std;
namespace json = rapidjson;

using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

template <> ShadowsocksOption parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(option::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
  assertTrue(v.HasMember(option::METHOD), PichiError::BAD_JSON, msg::MISSING_METHOD_FIELD);
  return {parse<string>(v[option::PASSWORD]), parse<CryptoMethod>(v[option::METHOD])};
}

json::Value toJson(ShadowsocksOption const& opt, Allocator& alloc)
{
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(option::PASSWORD, toJson(opt.password_, alloc), alloc);
  ret.AddMember(option::METHOD, toJson(opt.method_, alloc), alloc);
  return ret;
}

bool operator==(ShadowsocksOption const& lhs, ShadowsocksOption const& rhs)
{
  return lhs.password_ == rhs.password_ && lhs.method_ == rhs.method_;
}

template <> TunnelOption parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(option::DESTINATIONS), PichiError::BAD_JSON,
             msg::MISSING_DESTINATIONS_FIELD);
  assertTrue(v[option::DESTINATIONS].IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
  assertFalse(v[option::DESTINATIONS].Empty(), PichiError::BAD_JSON, msg::ARY_SIZE_ERROR);
  assertTrue(v.HasMember(option::BALANCE), PichiError::BAD_JSON, msg::MISSING_BALANCE_FIELD);
  return {accumulate(v[option::DESTINATIONS].Begin(), v[option::DESTINATIONS].End(),
                     vector<Endpoint>{},
                     [](auto&& sum, auto&& item) {
                       sum.push_back(parse<Endpoint>(item));
                       return move(sum);
                     }),
          parse<BalanceType>(v[option::BALANCE])};
}

json::Value toJson(TunnelOption const& opt, Allocator& alloc)
{
  assertFalse(opt.destinations_.empty());
  auto destinations = json::Value{json::kArrayType};
  for (auto&& dest : opt.destinations_) destinations.PushBack(toJson(dest, alloc), alloc);
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(option::DESTINATIONS, destinations, alloc);
  ret.AddMember(option::BALANCE, toJson(opt.balance_, alloc), alloc);
  return ret;
}

bool operator==(TunnelOption const& lhs, TunnelOption const& rhs)
{
  return lhs.destinations_ == rhs.destinations_ && lhs.balance_ == rhs.balance_;
}

template <> RejectOption parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(option::MODE), PichiError::BAD_JSON, msg::MISSING_MODE_FIELD);
  auto ret = RejectOption{};
  ret.mode_ = parse<DelayMode>(v[option::MODE]);
  if (ret.mode_ == DelayMode::FIXED) {
    ret.delay_ = v.HasMember(option::DELAY) ? parse<uint16_t>(v[option::DELAY]) : 0_u16;
    assertTrue(ret.delay_ <= 300_u16, PichiError::BAD_JSON, msg::DL_INVALID);
  }
  return ret;
}

json::Value toJson(RejectOption const& opt, Allocator& alloc)
{
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(option::MODE, toJson(opt.mode_, alloc), alloc);
  if (opt.mode_ == DelayMode::FIXED) {
    assertTrue(opt.delay_.has_value());
    assertTrue(opt.delay_ <= 300_u16);
    ret.AddMember(option::DELAY, *opt.delay_, alloc);
  }
  return ret;
}

bool operator==(RejectOption const& lhs, RejectOption const& rhs)
{
  return lhs.mode_ == rhs.mode_ && (lhs.mode_ != DelayMode::FIXED || lhs.delay_ == rhs.delay_);
}

template <> TrojanOption parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(option::REMOTE), PichiError::BAD_JSON, msg::MISSING_REMOTE_FIELD);
  return {parse<Endpoint>(v[option::REMOTE])};
}

json::Value toJson(TrojanOption const& opt, Allocator& alloc)
{
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(option::REMOTE, toJson(opt.remote_, alloc), alloc);
  return ret;
}

bool operator==(TrojanOption const& lhs, TrojanOption const& rhs)
{
  return lhs.remote_ == rhs.remote_;
}

template <> TlsIngressOption parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(tls::CERT_FILE), PichiError::BAD_JSON, msg::MISSING_CERT_FILE_FIELD);
  assertTrue(v.HasMember(tls::KEY_FILE), PichiError::BAD_JSON, msg::MISSING_KEY_FILE_FIELD);
  return {parse<string>(v[tls::CERT_FILE]), parse<string>(v[tls::KEY_FILE])};
}

json::Value toJson(TlsIngressOption const& opt, Allocator& alloc)
{
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(tls::CERT_FILE, toJson(opt.certFile_, alloc), alloc);
  ret.AddMember(tls::KEY_FILE, toJson(opt.keyFile_, alloc), alloc);
  return ret;
}

bool operator==(TlsIngressOption const& lhs, TlsIngressOption const& rhs)
{
  return lhs.keyFile_ == rhs.keyFile_ && lhs.certFile_ == rhs.certFile_;
}

template <> TlsEgressOption parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  auto ret = TlsEgressOption{};
  ret.insecure_ = v.HasMember(tls::INSECURE) ? parse<bool>(v[tls::INSECURE]) : false;
  if (v.HasMember(tls::SNI)) ret.sni_ = parse<string>(v[tls::SNI]);
  if (!ret.insecure_) {
    if (v.HasMember(tls::CA_FILE)) ret.caFile_ = parse<string>(v[tls::CA_FILE]);
    if (v.HasMember(tls::SERVER_NAME)) ret.serverName_ = parse<string>(v[tls::SERVER_NAME]);
  }
  return ret;
}

json::Value toJson(TlsEgressOption const& opt, Allocator& alloc)
{
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(tls::INSECURE, opt.insecure_, alloc);
  if (opt.sni_.has_value()) ret.AddMember(tls::SNI, toJson(*opt.sni_, alloc), alloc);
  if (!opt.insecure_) {
    if (opt.caFile_.has_value()) ret.AddMember(tls::CA_FILE, toJson(*opt.caFile_, alloc), alloc);
    if (opt.serverName_.has_value())
      ret.AddMember(tls::SERVER_NAME, toJson(*opt.serverName_, alloc), alloc);
  }
  return ret;
}

bool operator==(TlsEgressOption const& lhs, TlsEgressOption const& rhs)
{
  if (lhs.sni_ != rhs.sni_) return false;
  return lhs.insecure_ == rhs.insecure_ &&
         (lhs.insecure_ || (lhs.caFile_ == rhs.caFile_ && lhs.serverName_ == rhs.serverName_));
}

template <> WebsocketOption parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(websocket::PATH), PichiError::BAD_JSON, msg::MISSING_PATH_FIELD);
  auto ret = WebsocketOption{};
  ret.path_ = parse<string>(v[websocket::PATH]);
  if (v.HasMember(websocket::HOST)) ret.host_ = parse<string>(v[websocket::HOST]);
  return ret;
}

json::Value toJson(WebsocketOption const& opt, Allocator& alloc)
{
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(websocket::PATH, toJson(opt.path_, alloc), alloc);
  if (opt.host_.has_value()) ret.AddMember(websocket::HOST, toJson(*opt.host_, alloc), alloc);
  return ret;
}

bool operator==(WebsocketOption const& lhs, WebsocketOption const& rhs)
{
  return lhs.path_ == rhs.path_ && lhs.host_ == rhs.host_;
}

}  // namespace pichi::vo
