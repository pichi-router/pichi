#ifndef PICHI_VO_INGRESS_HPP
#define PICHI_VO_INGRESS_HPP

#include <optional>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <rapidjson/document.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace pichi::vo {

struct Ingress {
  AdapterType type_;
  std::string bind_ = {};
  uint16_t port_ = 0;
  std::optional<CryptoMethod> method_ = {};
  std::optional<std::string> password_ = {};
  std::unordered_map<std::string, std::string> credentials_ = {};
  std::optional<bool> tls_ = {};
  std::optional<std::string> certFile_ = {};
  std::optional<std::string> keyFile_ = {};
  std::vector<pichi::Endpoint> destinations_ = {};
  std::optional<BalanceType> balance_ = {};
};

extern rapidjson::Value toJson(Ingress const&, rapidjson::Document::AllocatorType&);

}  // namespace pichi::vo

#endif  // PICHI_VO_INGRESS_HPP
