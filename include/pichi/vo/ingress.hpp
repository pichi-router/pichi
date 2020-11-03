#ifndef PICHI_VO_INGRESS_HPP
#define PICHI_VO_INGRESS_HPP

#include <optional>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/vo/credential.hpp>
#include <pichi/vo/options.hpp>
#include <rapidjson/document.h>
#include <variant>
#include <vector>

namespace pichi::vo {

struct Ingress {
  using Credential =
      std::variant<UpIngressCredential, TrojanIngressCredential, VMessIngressCredential>;
  using Options = std::variant<TunnelOption, ShadowsocksOption, TrojanOption>;

  AdapterType type_;
  std::vector<Endpoint> bind_ = {};
  std::optional<Credential> credential_ = {};
  std::optional<Options> opt_ = {};
  std::optional<TlsIngressOption> tls_ = {};
  std::optional<WebsocketOption> websocket_ = {};
};

extern rapidjson::Value toJson(Ingress const&, rapidjson::Document::AllocatorType&);

extern bool operator==(Ingress const&, Ingress const&);

}  // namespace pichi::vo

#endif  // PICHI_VO_INGRESS_HPP
