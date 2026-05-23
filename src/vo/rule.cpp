#include "pichi/common/config.hpp"
#include <algorithm>
#include <iterator>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/rule.hpp>
#include <pichi/vo/to_json.hpp>
#include <ranges>

namespace json  = rapidjson;
namespace rngs  = std::ranges;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value toJson(Rule const& rvo, Allocator& alloc)
{
  auto rule = json::Value{};
  rule.SetObject();
  if (!rvo.range_.empty())
    rule.AddMember(
        rule::RANGE,
        toJson(rngs::begin(rvo.range_), rngs::end(rvo.range_), alloc),
        alloc
    );
  if (!rvo.range_nr_.empty())
    rule.AddMember(
        rule::RANGE_NR,
        toJson(rngs::begin(rvo.range_nr_), rngs::end(rvo.range_nr_), alloc),
        alloc
    );
  if (!rvo.ingress_.empty())
    rule.AddMember(
        rule::INGRESS,
        toJson(rngs::begin(rvo.ingress_), rngs::end(rvo.ingress_), alloc),
        alloc
    );
  if (!rvo.type_.empty())
    rule.AddMember(rule::TYPE, toJson(rngs::begin(rvo.type_), rngs::end(rvo.type_), alloc), alloc);
  if (!rvo.pattern_.empty())
    rule.AddMember(
        rule::PATTERN,
        toJson(rngs::begin(rvo.pattern_), rngs::end(rvo.pattern_), alloc),
        alloc
    );
  if (!rvo.domain_.empty())
    rule.AddMember(
        rule::DOMAIN_NAME,
        toJson(rngs::begin(rvo.domain_), rngs::end(rvo.domain_), alloc),
        alloc
    );
  if (!rvo.country_.empty())
    rule.AddMember(
        rule::COUNTRY,
        toJson(rngs::begin(rvo.country_), rngs::end(rvo.country_), alloc),
        alloc
    );
  if (!rvo.country_nr_.empty())
    rule.AddMember(
        rule::COUNTRY_NR,
        toJson(rngs::begin(rvo.country_nr_), rngs::end(rvo.country_nr_), alloc),
        alloc
    );
  return rule;
}

template <> Rule parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);

  auto rvo              = Rule{};
  auto parseString      = [](auto&& v) { return parse<std::string>(v); };
  auto parseAdapterType = [](auto&& v) { return parse<AdapterType>(v); };

  parseArray(v, rule::RANGE, std::back_inserter(rvo.range_), parseString);
  parseArray(v, rule::RANGE_NR, std::back_inserter(rvo.range_nr_), parseString);
  parseArray(v, rule::INGRESS, std::back_inserter(rvo.ingress_), parseString);
  parseArray(v, rule::TYPE, std::back_inserter(rvo.type_), parseAdapterType);
  parseArray(v, rule::PATTERN, std::back_inserter(rvo.pattern_), parseString);
  parseArray(v, rule::DOMAIN_NAME, std::back_inserter(rvo.domain_), parseString);
  parseArray(v, rule::COUNTRY, std::back_inserter(rvo.country_), parseString);
  parseArray(v, rule::COUNTRY_NR, std::back_inserter(rvo.country_nr_), parseString);

  return rvo;
}

bool operator==(Rule const& lhs, Rule const& rhs)
{
  return rngs::equal(lhs.range_, rhs.range_) && rngs::equal(lhs.range_nr_, rhs.range_nr_) &&
         rngs::equal(lhs.ingress_, rhs.ingress_) && rngs::equal(lhs.type_, rhs.type_) &&
         rngs::equal(lhs.pattern_, rhs.pattern_) && rngs::equal(lhs.domain_, rhs.domain_) &&
         rngs::equal(lhs.country_, rhs.country_) && rngs::equal(lhs.country_nr_, rhs.country_nr_);
}

}  // namespace pichi::vo
