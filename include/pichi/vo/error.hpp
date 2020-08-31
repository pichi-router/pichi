#ifndef PICHI_VO_ERROR_HPP
#define PICHI_VO_ERROR_HPP

#include <rapidjson/document.h>
#include <string_view>

namespace pichi::vo {

struct Error {
  std::string_view message_;
};

extern rapidjson::Value toJson(Error const&, rapidjson::Document::AllocatorType&);

}  // namespace pichi::vo

#endif  // PICHI_VO_ERROR_HPP
