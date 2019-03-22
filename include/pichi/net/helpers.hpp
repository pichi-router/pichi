#ifndef PICHI_NET_HELPERS_HPP
#define PICHI_NET_HELPERS_HPP

#include <algorithm>
#include <functional>
#include <iterator>
#include <pichi/buffer.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>
#include <string_view>
#include <type_traits>

namespace std {

inline string to_string(string_view sv) { return {cbegin(sv), cend(sv)}; }

} // namespace std

namespace pichi::net {

template <typename Int> void hton(Int src, MutableBuffer<uint8_t> dst)
{
  static_assert(std::is_integral_v<Int>, "input type must be integral.");
  assert(sizeof(Int) <= dst.size());
  auto p = reinterpret_cast<uint8_t*>(&src);
  std::reverse_copy(p, p + sizeof(Int), std::begin(dst));
}

template <typename Int> Int ntoh(ConstBuffer<uint8_t> src)
{
  static_assert(std::is_integral_v<Int>, "output type must be integral.");
  assert(src.size() >= sizeof(Int));

  Int dst;
  std::reverse_copy(std::cbegin(src), std::cend(src), reinterpret_cast<uint8_t*>(&dst));
  return dst;
}

extern size_t serializeEndpoint(Endpoint const&, MutableBuffer<uint8_t>);
extern Endpoint parseEndpoint(std::function<void(MutableBuffer<uint8_t>)>);
extern Endpoint::Type detectHostType(std::string_view);
extern Endpoint makeEndpoint(std::string_view, uint16_t);
extern Endpoint makeEndpoint(std::string_view, std::string_view);

} // namespace pichi::net

#endif
