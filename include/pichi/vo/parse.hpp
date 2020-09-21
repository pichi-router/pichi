#ifndef PICHI_VO_PARSE_HPP
#define PICHI_VO_PARSE_HPP

#include <algorithm>
#include <iterator>
#include <pichi/common/asserts.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/vo/messages.hpp>
#include <rapidjson/document.h>
#include <stdint.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pichi::vo {

template <typename T> T parse(rapidjson::Value const&);

extern uint16_t parsePort(rapidjson::Value const&);
extern std::string parseNameOrPassword(rapidjson::Value const&);
extern std::vector<Endpoint> parseDestinantions(rapidjson::Value const&);

template <typename T> T parse(std::string_view src)
{
  auto doc = rapidjson::Document{};
  doc.Parse(src.data(), src.size());
  assertFalse(doc.HasParseError(), PichiError::BAD_JSON, "JSON syntax error");
  return parse<T>(doc);
}

template <typename Parser> auto parsePair(rapidjson::Value const& v, Parser&& parseItem)
{
  static_assert(std::is_invocable_r_v<std::string, Parser, rapidjson::Value const&>);
  assertTrue(v.IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
  auto array = v.GetArray();
  assertTrue(array.Size() == 2, PichiError::BAD_JSON, msg::PAIR_TYPE_ERROR);
  return std::make_pair(parseItem(array[0]), parseItem(array[1]));
}

template <typename OutputIt, typename T, typename Converter>
void parseArray(rapidjson::Value const& root, T const& key, OutputIt out, Converter&& convert)
{
  if (!root.HasMember(key)) return;
  assertTrue(root[key].IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
  auto array = root[key].GetArray();
  std::transform(std::begin(array), std::end(array), out, std::forward<Converter>(convert));
}

}  // namespace pichi::vo

#endif  // PICHI_VO_PARSE_HPP
