#include <iterator>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/rule.hpp>
#include <pichi/vo/to_json.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value toJson(Rule const& rvo, Allocator& alloc)
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

template <> Rule parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);

  auto rvo = Rule{};
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

bool operator==(Rule const& lhs, Rule const& rhs)
{
  return equal(begin(lhs.range_), end(lhs.range_), begin(rhs.range_), end(rhs.range_)) &&
         equal(begin(lhs.ingress_), end(lhs.ingress_), begin(rhs.ingress_), end(rhs.ingress_)) &&
         equal(begin(lhs.type_), end(lhs.type_), begin(rhs.type_), end(rhs.type_)) &&
         equal(begin(lhs.pattern_), end(lhs.pattern_), begin(rhs.pattern_), end(rhs.pattern_)) &&
         equal(begin(lhs.domain_), end(lhs.domain_), begin(rhs.domain_), end(rhs.domain_)) &&
         equal(begin(lhs.country_), end(lhs.country_), begin(rhs.country_), end(rhs.country_));
}

}  // namespace pichi::vo
