#include <pichi/common/config.hpp>
// Include config.hpp first
#include <iterator>
#include <pichi/api/egress_manager.hpp>
#include <pichi/common/asserts.hpp>

using namespace std;

namespace pichi::api {

static auto DEFAULT_EGRESS_NAME = "direct";

EgressManager::EgressManager() : c_{{DEFAULT_EGRESS_NAME, {}}}
{
  c_[DEFAULT_EGRESS_NAME].type_ = AdapterType::DIRECT;
}

void EgressManager::update(string const& name, VO vo) { c_[name] = move(vo); }

void EgressManager::erase(string_view name)
{
  auto it = find(name);
  if (it != end()) c_.erase(it);
}

EgressManager::ConstIterator EgressManager::begin() const noexcept { return cbegin(c_); }

EgressManager::ConstIterator EgressManager::end() const noexcept { return cend(c_); }

EgressManager::ConstIterator EgressManager::find(string_view name) const { return c_.find(name); }

}  // namespace pichi::api
