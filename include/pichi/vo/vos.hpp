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

struct ErrorVO {
  std::string_view message_;
};

extern rapidjson::Value toJson(ErrorVO const&, rapidjson::Document::AllocatorType&);

}  // namespace pichi::vo

#endif  // PICHI_VO_VOS_HPP
