#ifndef PICHI_VO_OPTIONS_HPP
#define PICHI_VO_OPTIONS_HPP

#include <optional>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <rapidjson/document.h>
#include <string>
#include <vector>

namespace pichi::vo {

struct ShadowsocksOption {
  std::string password_;
  CryptoMethod method_;
};

extern rapidjson::Value toJson(ShadowsocksOption const&, rapidjson::Document::AllocatorType&);
extern bool operator==(ShadowsocksOption const&, ShadowsocksOption const&);

struct TunnelOption {
  std::vector<Endpoint> destinations_;
  BalanceType balance_;
};

extern rapidjson::Value toJson(TunnelOption const&, rapidjson::Document::AllocatorType&);
extern bool operator==(TunnelOption const&, TunnelOption const&);

struct RejectOption {
  DelayMode mode_;
  std::optional<uint16_t> delay_;
};

extern rapidjson::Value toJson(RejectOption const&, rapidjson::Document::AllocatorType&);
extern bool operator==(RejectOption const&, RejectOption const&);

struct TrojanOption {
  Endpoint remote_;
};

extern rapidjson::Value toJson(TrojanOption const&, rapidjson::Document::AllocatorType&);
extern bool operator==(TrojanOption const&, TrojanOption const&);

struct TlsIngressOption {
  std::string certFile_;
  std::string keyFile_;
};

extern rapidjson::Value toJson(TlsIngressOption const&, rapidjson::Document::AllocatorType&);
extern bool operator==(TlsIngressOption const&, TlsIngressOption const&);

struct TlsEgressOption {
  bool insecure_;
  std::optional<std::string> caFile_;
  std::optional<std::string> serverName_;
  std::optional<std::string> sni_;
};

extern rapidjson::Value toJson(TlsEgressOption const&, rapidjson::Document::AllocatorType&);
extern bool operator==(TlsEgressOption const&, TlsEgressOption const&);

struct WebsocketOption {
  std::string path_;
  std::optional<std::string> host_;
};

extern rapidjson::Value toJson(WebsocketOption const&, rapidjson::Document::AllocatorType&);
extern bool operator==(WebsocketOption const&, WebsocketOption const&);

}  // namespace pichi::vo

#endif  // PICHI_VO_OPTIONS_HPP
