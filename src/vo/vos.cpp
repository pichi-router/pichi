#include <numeric>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/messages.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <pichi/vo/vos.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value toJson(RuleVO const& rvo, Allocator& alloc)
{
  auto rule = json::Value{};
  rule.SetObject();
  if (!rvo.range_.empty())
    rule.AddMember(rule::RANGE, toJson(begin(rvo.range_), end(rvo.range_), alloc), alloc);
  if (!rvo.ingress_.empty())
    rule.AddMember(rule::INGRESS, toJson(begin(rvo.ingress_), end(rvo.ingress_), alloc), alloc);
  if (!rvo.type_.empty())
    rule.AddMember(rule::TYPE, toJson(begin(rvo.type_), end(rvo.type_), alloc), alloc);
  if (!rvo.pattern_.empty())
    rule.AddMember(rule::PATTERN, toJson(begin(rvo.pattern_), end(rvo.pattern_), alloc), alloc);
  if (!rvo.domain_.empty())
    rule.AddMember(rule::DOMAIN_NAME, toJson(begin(rvo.domain_), end(rvo.domain_), alloc), alloc);
  if (!rvo.country_.empty())
    rule.AddMember(rule::COUNTRY, toJson(begin(rvo.country_), end(rvo.country_), alloc), alloc);
  return rule;
}

json::Value toJson(RouteVO const& rvo, Allocator& alloc)
{
  // "default" is required when sending to clients
  assertTrue(rvo.default_.has_value(), PichiError::MISC);
  assertFalse(rvo.default_->empty(), PichiError::MISC);

  auto route = json::Value{};
  route.SetObject();
  route.AddMember(route::DEFAULT, toJson(*rvo.default_, alloc), alloc);

  auto rules = json::Value{};
  rules.SetArray();
  for_each(cbegin(rvo.rules_), cend(rvo.rules_), [&](auto&& item) {
    auto rule = json::Value{};
    rule.SetArray();
    for_each(cbegin(item.first), cend(item.first),
             [&](auto&& name) { rule.PushBack(toJson(name, alloc), alloc); });
    rule.PushBack(toJson(item.second, alloc), alloc);
    rules.PushBack(move(rule), alloc);
  });
  route.AddMember(route::RULES, move(rules), alloc);
  return route;
}

json::Value toJson(ErrorVO const& evo, Allocator& alloc)
{
  using StringRef = json::Value::StringRefType;
  using SizeType = json::SizeType;

  auto error = json::Value{};
  error.SetObject();
  error.AddMember(error::MESSAGE,
                  StringRef{evo.message_.data(), static_cast<SizeType>(evo.message_.size())},
                  alloc);
  return error;
}

template <> RuleVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);

  auto rvo = RuleVO{};
  auto parseString = [](auto&& v) { return parse<string>(v); };
  auto parseAdapterType = [](auto&& v) { return parse<AdapterType>(v); };

  parseArray(v, rule::RANGE, back_inserter(rvo.range_), parseString);
  parseArray(v, rule::INGRESS, back_inserter(rvo.ingress_), parseString);
  parseArray(v, rule::TYPE, back_inserter(rvo.type_), parseAdapterType);
  parseArray(v, rule::PATTERN, back_inserter(rvo.pattern_), parseString);
  parseArray(v, rule::DOMAIN_NAME, back_inserter(rvo.domain_), parseString);
  parseArray(v, rule::COUNTRY, back_inserter(rvo.country_), parseString);

  return rvo;
}

template <> RouteVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);

  auto rvo = RouteVO{};

  if (v.HasMember(route::DEFAULT)) rvo.default_.emplace(parse<string>(v[route::DEFAULT]));

  parseArray(v, route::RULES, back_inserter(rvo.rules_), [](auto&& v) {
    assertTrue(v.IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
    auto array = v.GetArray();
    assertTrue(array.Size() >= 2, PichiError::BAD_JSON, msg::ARY_SIZE_ERROR);
    auto first = array.Begin();
    auto last = first + (array.Size() - 1);
    return make_pair(accumulate(first, last, vector<string>{},
                                [](auto&& full, auto&& item) {
                                  full.push_back(parse<string>(item));
                                  return move(full);
                                }),
                     parse<string>(*last));
  });

  return rvo;
}

}  // namespace pichi::vo
