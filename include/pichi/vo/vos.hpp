#ifndef PICHI_VO_VOS_HPP
#define PICHI_VO_VOS_HPP

#include <algorithm>
#include <limits>
#include <pichi/common/asserts.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/vo/iterator.hpp>
#include <rapidjson/document.h>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pichi::vo {

struct RouteVO {
  std::optional<std::string> default_ = {};
  std::vector<std::pair<std::vector<std::string>, std::string>> rules_ = {};
};

struct ErrorVO {
  std::string_view message_;
};

extern rapidjson::Value toJson(RouteVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(ErrorVO const&, rapidjson::Document::AllocatorType&);

}  // namespace pichi::vo

#endif  // PICHI_VO_VOS_HPP
