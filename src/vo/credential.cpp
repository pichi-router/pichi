#include "pichi/common/config.hpp"
#include <numeric>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/credential.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/messages.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

namespace json  = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

template <typename R, typename F> R parseArray(json::Value const& v, F&& f)
{
  assertTrue(v.IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
  auto&& all = v.GetArray();
  assertFalse(all.Empty(), PichiError::BAD_JSON, msg::ARY_SIZE_ERROR);
  return std::accumulate(std::cbegin(all), std::cend(all), R{}, std::forward<F>(f));
}

template <typename Credential, typename Converter>
json::Value toJson(Credential const& cred, Converter&& convert, Allocator& alloc)
{
  assertFalse(cred.credential_.empty());
  auto ret = json::Value{json::kArrayType};
  for (auto&& item : cred.credential_) ret.PushBack(convert(item, alloc), alloc);
  return ret;
}

namespace up {

static auto toJson(std::pair<std::string, std::string> const& p, Allocator& alloc)
{
  auto&& [username, password] = p;
  auto item                   = json::Value{json::kObjectType};
  item.AddMember(credential::USERNAME, vo::toJson(username, alloc), alloc);
  item.AddMember(credential::PASSWORD, vo::toJson(password, alloc), alloc);
  return item;
}

}  // namespace up

template <> UpIngressCredential parse(json::Value const& v)
{
  return {parseArray<std::unordered_map<std::string, std::string>>(v, [](auto&& sum, auto&& item) {
    assertTrue(item.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
    assertTrue(item.HasMember(credential::USERNAME), PichiError::BAD_JSON, msg::MISSING_UN_FIELD);
    assertTrue(item.HasMember(credential::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
    auto [_, inserted] = sum.insert_or_assign(
        parse<std::string>(item[credential::USERNAME]),
        parse<std::string>(item[credential::PASSWORD])
    );
    assertTrue(inserted, PichiError::SEMANTIC_ERROR, msg::DUPLICATED_ITEMS);
    return move(sum);
  })};
}

json::Value toJson(UpIngressCredential const& cred, Allocator& alloc)
{
  return toJson(cred, up::toJson, alloc);
}

template <> UpEgressCredential parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
  assertTrue(v.HasMember(credential::USERNAME), PichiError::BAD_JSON, msg::MISSING_UN_FIELD);
  assertTrue(v.HasMember(credential::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
  return {make_pair(
      parse<std::string>(v[credential::USERNAME]),
      parse<std::string>(v[credential::PASSWORD])
  )};
}

json::Value toJson(UpEgressCredential const& cred, Allocator& alloc)
{
  return up::toJson(cred.credential_, alloc);
}

namespace trojan {

static auto toJson(std::string_view pw, Allocator& alloc)
{
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(credential::PASSWORD, vo::toJson(pw, alloc), alloc);
  return ret;
}

}  // namespace trojan

template <> TrojanIngressCredential parse(json::Value const& v)
{
  return {parseArray<std::unordered_set<std::string>>(v, [](auto&& sum, auto&& item) {
    assertTrue(item.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
    assertTrue(item.HasMember(credential::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
    auto [_, inserted] = sum.insert(parse<std::string>(item[credential::PASSWORD]));
    assertTrue(inserted, PichiError::SEMANTIC_ERROR, msg::DUPLICATED_ITEMS);
    return move(sum);
  })};
}

json::Value toJson(TrojanIngressCredential const& cred, Allocator& alloc)
{
  return toJson(cred, trojan::toJson, alloc);
}

template <> TrojanEgressCredential parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(credential::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
  return {parse<std::string>(v[credential::PASSWORD])};
}

json::Value toJson(TrojanEgressCredential const& cred, Allocator& alloc)
{
  return trojan::toJson(cred.credential_, alloc);
}

bool operator==(UpIngressCredential const& lhs, UpIngressCredential const& rhs)
{
  return lhs.credential_ == rhs.credential_;
}

bool operator==(UpEgressCredential const& lhs, UpEgressCredential const& rhs)
{
  return lhs.credential_ == rhs.credential_;
}

bool operator==(TrojanIngressCredential const& lhs, TrojanIngressCredential const& rhs)
{
  return lhs.credential_ == rhs.credential_;
}

bool operator==(TrojanEgressCredential const& lhs, TrojanEgressCredential const& rhs)
{
  return lhs.credential_ == rhs.credential_;
}

}  // namespace pichi::vo
