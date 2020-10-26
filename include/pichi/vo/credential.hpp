#ifndef PICHI_VO_CREDENTIAL_HPP
#define PICHI_VO_CREDENTIAL_HPP

#include <pichi/common/enumerations.hpp>
#include <rapidjson/document.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace pichi::vo {

struct UpIngressCredential {
  std::unordered_map<std::string, std::string> credential_;
};

struct UpEgressCredential {
  std::pair<std::string, std::string> credential_;
};

extern rapidjson::Value toJson(UpIngressCredential const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(UpEgressCredential const&, rapidjson::Document::AllocatorType&);

struct TrojanIngressCredential {
  std::unordered_set<std::string> credential_;
};

struct TrojanEgressCredential {
  std::string credential_;
};

extern rapidjson::Value toJson(TrojanIngressCredential const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(TrojanEgressCredential const&, rapidjson::Document::AllocatorType&);

struct VMessIngressCredential {
  std::unordered_map<std::string, uint16_t> credential_;
};

struct VMessEgressCredential {
  std::string uuid_;
  uint16_t alter_id_;
  VMessSecurity security_;
};

extern rapidjson::Value toJson(VMessIngressCredential const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(VMessEgressCredential const&, rapidjson::Document::AllocatorType&);

extern bool operator==(UpIngressCredential const&, UpIngressCredential const&);
extern bool operator==(UpEgressCredential const&, UpEgressCredential const&);
extern bool operator==(TrojanIngressCredential const&, TrojanIngressCredential const&);
extern bool operator==(TrojanEgressCredential const&, TrojanEgressCredential const&);
extern bool operator==(VMessIngressCredential const&, VMessIngressCredential const&);
extern bool operator==(VMessEgressCredential const&, VMessEgressCredential const&);

}  // namespace pichi::vo

#endif  // PICHI_VO_CREDENTIAL_HPP
