#ifndef PICHI_VO_ROUTE_HPP
#define PICHI_VO_ROUTE_HPP

#include <optional>
#include <rapidjson/document.h>
#include <string>
#include <utility>
#include <vector>

namespace pichi::vo {

struct Route {
  std::optional<std::string> default_ = {};
  std::vector<std::pair<std::vector<std::string>, std::string>> rules_ = {};
};

extern rapidjson::Value toJson(Route const&, rapidjson::Document::AllocatorType&);

extern bool operator==(Route const&, Route const&);

}  // namespace pichi::vo

#endif  // PICHI_VO_ROUTE_HPP
