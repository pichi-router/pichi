#include <pichi/common/config.hpp>
// Include config.hpp first
#include <iterator>
#include <pichi/api/egress_manager.hpp>

using namespace std;

namespace pichi::api {

void EgressManager::update(string const& name, VO vo)
{
#ifndef ENABLE_TLS
  assertFalse(vo.tls_.has_value() && *vo.tls_, PichiError::SEMANTIC_ERROR, "TLS not supported");
#endif  // ENABLE_TLS
  c_[name] = move(vo);
}

void EgressManager::erase(string_view name)
{
  auto it = find(name);
  if (it != end()) c_.erase(it);
}

EgressManager::ConstIterator EgressManager::begin() const noexcept { return cbegin(c_); }

EgressManager::ConstIterator EgressManager::end() const noexcept { return cend(c_); }

EgressManager::ConstIterator EgressManager::find(string_view name) const { return c_.find(name); }

}  // namespace pichi::api