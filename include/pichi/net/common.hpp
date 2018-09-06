#ifndef PICHI_NET_COMMON_HPP
#define PICHI_NET_COMMON_HPP

#include <string>

namespace pichi {
namespace net {

enum class AdapterType { DIRECT, REJECT, SOCKS5, HTTP, SS };

struct Endpoint {
  enum class Type { DOMAIN_NAME, IPV4, IPV6 } type_;
  std::string host_;
  std::string port_;
};

size_t const MAX_FRAME_SIZE = 0x3fff;

} // namespace net
} // namespace pichi

#endif
