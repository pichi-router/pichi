#include <pichi/api/egress_manager.hpp>

using namespace std;

namespace pichi::api {

void EgressManager::update(string const& name, EgressVO vo) { c_[name] = move(vo); }

void EgressManager::erase(string_view name)
{
  auto it = find(name);
  if (it != end()) c_.erase(it);
}

EgressManager::ConstIterator EgressManager::begin() const noexcept { return cbegin(c_); }

EgressManager::ConstIterator EgressManager::end() const noexcept { return cend(c_); }

EgressManager::ConstIterator EgressManager::find(string_view name) const { return c_.find(name); }

} // namespace pichi::api