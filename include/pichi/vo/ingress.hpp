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

namespace detail {

using Options = std::variant<TunnelOption, ShadowsocksOption, TrojanOption>;
using Credential =
    std::variant<up::IngressCredential, trojan::IngressCredential, vmess::IngressCredential>;
using Websocket = WebsocketOption;
using Tls = TlsIngressOption;

}  // namespace detail

struct Ingress {
  AdapterType type_;
  std::vector<Endpoint> bind_ = {};
  std::optional<detail::Options> opt_ = {};
  std::optional<detail::Tls> tls_ = {};
  std::optional<detail::Credential> credential_ = {};
  std::optional<detail::Websocket> websocket_ = {};
};

extern rapidjson::Value toJson(Ingress const&, rapidjson::Document::AllocatorType&);

extern bool operator==(Ingress const&, Ingress const&);

}  // namespace pichi::vo

#endif  // PICHI_VO_INGRESS_HPP
