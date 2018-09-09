#ifndef PICHI_URI_HPP
#define PICHI_URI_HPP

#include <string_view>

namespace pichi {

struct Uri {
  Uri(std::string_view);

  std::string_view all_;
  std::string_view scheme_;
  std::string_view host_;
  std::string_view port_;
  std::string_view suffix_;
  std::string_view path_;
  std::string_view query_;
};

struct HostAndPort {
  HostAndPort(std::string_view);

  std::string_view host_;
  std::string_view port_;
};

} // namespace pichi

#endif // PICHI_URI_HPP
