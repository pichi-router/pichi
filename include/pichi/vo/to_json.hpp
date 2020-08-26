#ifndef PICHI_VO_TO_JSON_HPP
#define PICHI_VO_TO_JSON_HPP

#include <algorithm>
#include <pichi/common/asserts.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/vo/iterator.hpp>
#include <rapidjson/document.h>
#include <string>
#include <string_view>
#include <type_traits>

namespace pichi::vo {

extern rapidjson::Value toJson(AdapterType, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(CryptoMethod, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(DelayMode, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(BalanceType, rapidjson::Document::AllocatorType&);
extern rapidjson::Value toJson(std::string_view, rapidjson::Document::AllocatorType&);

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

}  // namespace pichi::vo

#endif  // PICHI_VO_TO_JSON_HPP
