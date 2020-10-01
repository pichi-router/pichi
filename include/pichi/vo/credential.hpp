#ifndef PICHI_VO_CREDENTIAL_HPP
#define PICHI_VO_CREDENTIAL_HPP

#include <pichi/common/enumerations.hpp>
#include <rapidjson/document.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace pichi::vo {

namespace up {

struct IngressCredential {
  std::unordered_map<std::string, std::string> credential_;
};

struct EgressCredential {
  std::pair<std::string, std::string> credential_;
};

}  // namespace up

extern rapidjson::Value toJson(up::IngressCredential const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(up::EgressCredential const&, rapidjson::Document::AllocatorType&);

namespace trojan {

struct IngressCredential {
  std::unordered_set<std::string> credential_;
};

struct EgressCredential {
  std::string credential_;
};

}  // namespace trojan

extern rapidjson::Value toJson(trojan::IngressCredential const&,
                               rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(trojan::EgressCredential const&,
                               rapidjson::Document::AllocatorType&);

namespace vmess {

struct IngressCredential {
  std::unordered_map<std::string, uint16_t> credential_;
};

struct EgressCredential {
  std::string uuid_;
  uint16_t alter_id_;
  VMessSecurity security_;
};

}  // namespace vmess

extern rapidjson::Value toJson(vmess::IngressCredential const&,
                               rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(vmess::EgressCredential const&, rapidjson::Document::AllocatorType&);

extern bool operator==(up::IngressCredential const&, up::IngressCredential const&);
extern bool operator==(up::EgressCredential const&, up::EgressCredential const&);
extern bool operator==(trojan::IngressCredential const&, trojan::IngressCredential const&);
extern bool operator==(trojan::EgressCredential const&, trojan::EgressCredential const&);
extern bool operator==(vmess::IngressCredential const&, vmess::IngressCredential const&);
extern bool operator==(vmess::EgressCredential const&, vmess::EgressCredential const&);

}  // namespace pichi::vo

#endif  // PICHI_VO_CREDENTIAL_HPP
