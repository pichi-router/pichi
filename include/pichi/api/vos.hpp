#ifndef PICHI_API_VOS_HPP
#define PICHI_API_VOS_HPP

#include <algorithm>
#include <limits>
#include <pichi/api/iterator.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/crypto/method.hpp>
#include <rapidjson/document.h>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pichi::api {

enum class DelayMode { RANDOM, FIXED };
enum class BalanceType { RANDOM, ROUND_ROBIN, LEAST_CONN };

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

extern rapidjson::Value toJson(AdapterType, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(CryptoMethod, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(DelayMode, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(BalanceType, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(std::string_view, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(IngressVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(EgressVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(RuleVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(RouteVO const&, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(ErrorVO const&, rapidjson::Document::AllocatorType&);

template <typename InputIt>
auto toJson(InputIt first, InputIt last, rapidjson::Document::AllocatorType& alloc)
{
  auto ret = rapidjson::Value{};
  if constexpr (IsPairV<std::decay_t<typename std::iterator_traits<InputIt>::value_type>>) {
    ret.SetObject();
    std::for_each(first, last, [&ret, &alloc](auto&& i) {
      assertFalse(i.first.empty(), PichiError::MISC);
      ret.AddMember(toJson(i.first, alloc), toJson(i.second, alloc), alloc);
    });
  }
  else if constexpr (std::is_same_v<
                         std::decay_t<typename std::iterator_traits<InputIt>::value_type>,
                         Endpoint>) {
    ret.SetObject();
    std::for_each(first, last, [&ret, &alloc](auto&& endpoint) {
      auto port = std::stoi(endpoint.port_);
      assertTrue(port > 0 && port <= std::numeric_limits<uint16_t>::max());
      ret.AddMember(toJson(endpoint.host_, alloc), rapidjson::Value{port}, alloc);
    });
  }
  else {
    ret.SetArray();
    std::for_each(first, last, [&ret, &alloc](auto&& i) { ret.PushBack(toJson(i, alloc), alloc); });
  }
  return ret;
}

template <typename VO> VO parse(rapidjson::Value const&);

template <typename VO> VO parse(std::string_view src)
{
  auto doc = rapidjson::Document{};
  doc.Parse(src.data(), src.size());
  assertFalse(doc.HasParseError(), PichiError::BAD_JSON, "JSON syntax error");
  return parse<VO>(doc);
}

}  // namespace pichi::api

#endif  // PICHI_API_VOS_HPP
