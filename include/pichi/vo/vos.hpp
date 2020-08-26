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

struct IngressVO {
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

struct EgressVO {
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

struct RuleVO {
  std::vector<std::string> range_ = {};
  std::vector<std::string> ingress_ = {};
  std::vector<AdapterType> type_ = {};
  std::vector<std::string> pattern_ = {};
  std::vector<std::string> domain_ = {};
  std::vector<std::string> country_ = {};
};

struct RouteVO {
  std::optional<std::string> default_ = {};
  std::vector<std::pair<std::vector<std::string>, std::string>> rules_ = {};
};

struct ErrorVO {
  std::string_view message_;
};

extern rapidjson::Value toJson(IngressVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(EgressVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(RuleVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(RouteVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(ErrorVO const&, rapidjson::Document::AllocatorType&);

}  // namespace pichi::vo

#endif  // PICHI_VO_VOS_HPP
