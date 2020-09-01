#ifndef PICHI_VO_RULE_HPP
#define PICHI_VO_RULE_HPP

#include <pichi/common/enumerations.hpp>
#include <rapidjson/document.h>
#include <string>
#include <vector>

namespace pichi::vo {

struct Rule {
  std::vector<std::string> range_ = {};
  std::vector<std::string> ingress_ = {};
  std::vector<AdapterType> type_ = {};
  std::vector<std::string> pattern_ = {};
  std::vector<std::string> domain_ = {};
  std::vector<std::string> country_ = {};
};

extern rapidjson::Value toJson(Rule const&, rapidjson::Document::AllocatorType&);

}  // namespace pichi::vo

#endif  // PICHI_VO_RULE_HPP
