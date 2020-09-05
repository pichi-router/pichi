#ifndef PICHI_VO_EGRESS_HPP
#define PICHI_VO_EGRESS_HPP

#include <optional>
#include <pichi/common/enumerations.hpp>
#include <rapidjson/document.h>
#include <stdint.h>
#include <string>
#include <utility>

namespace pichi::vo {

struct Egress {
  AdapterType type_;
  std::optional<std::string> host_ = {};
  std::optional<uint16_t> port_ = {};
  std::optional<CryptoMethod> method_ = {};
  std::optional<std::string> password_ = {};
  std::optional<DelayMode> mode_ = {};
  std::optional<uint16_t> delay_ = {};
  std::optional<std::pair<std::string, std::string>> credential_ = {};
  std::optional<bool> tls_ = {};
  std::optional<bool> insecure_ = {};
  std::optional<std::string> caFile_ = {};
};

extern rapidjson::Value toJson(Egress const&, rapidjson::Document::AllocatorType&);

}  // namespace pichi::vo

#endif  // PICHI_VO_EGRESS_HPP
