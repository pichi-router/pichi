#ifndef PICHI_VO_EGRESS_HPP
#define PICHI_VO_EGRESS_HPP

#include <optional>
#include <pichi/common/enumerations.hpp>
#include <pichi/vo/credential.hpp>
#include <pichi/vo/options.hpp>
#include <rapidjson/document.h>
#include <variant>

namespace pichi::vo {

struct Egress {
  using Credential =
      std::variant<UpEgressCredential, TrojanEgressCredential, VMessEgressCredential>;
  using Option = std::variant<RejectOption, ShadowsocksOption>;

  AdapterType type_;
  std::optional<Endpoint> server_;
  std::optional<Credential> credential_;
  std::optional<Option> opt_;
  std::optional<TlsEgressOption> tls_;
  std::optional<WebsocketOption> websocket_;
};

extern rapidjson::Value toJson(Egress const&, rapidjson::Document::AllocatorType&);

extern bool operator==(Egress const&, Egress const&);

}  // namespace pichi::vo

#endif  // PICHI_VO_EGRESS_HPP
