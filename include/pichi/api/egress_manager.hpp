#ifndef PICHI_API_EGRESS_MANAGER_HPP
#define PICHI_API_EGRESS_MANAGER_HPP

#include <map>
#include <pichi/vo/vos.hpp>
#include <string>
#include <string_view>

namespace pichi::api {

class EgressManager {
public:
  using VO = vo::EgressVO;

private:
  using Container = std::map<std::string, VO, std::less<>>;
  using ConstIterator = typename Container::const_iterator;

public:
  EgressManager() = default;

  void update(std::string const&, VO);
  void erase(std::string_view);

  ConstIterator begin() const noexcept;
  ConstIterator end() const noexcept;
  ConstIterator find(std::string_view) const;

private:
  Container c_ = {{"direct", {AdapterType::DIRECT}}};
};

}  // namespace pichi::api

#endif  // PICHI_API_EGRESS_MANAGER_HPP
