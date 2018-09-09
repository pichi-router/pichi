#ifndef PICHI_API_EGRESS_HPP
#define PICHI_API_EGRESS_HPP

#include <functional>
#include <map>
#include <pichi/api/rest.hpp>
#include <string>
#include <string_view>

namespace pichi::api {

class Egress {
public:
  using VO = OutboundVO;

private:
  using Container = std::map<std::string, OutboundVO, std::less<>>;
  using ConstIterator = typename Container::const_iterator;

public:
  Egress() = default;

  void update(std::string const&, OutboundVO);
  void erase(std::string_view);

  ConstIterator begin() const noexcept;
  ConstIterator end() const noexcept;
  ConstIterator find(std::string_view) const;

private:
  Container c_ = {{"direct", {AdapterType::DIRECT}}};
};

} // namespace pichi::api

#endif // PICHI_API_EGRESS_HPP
