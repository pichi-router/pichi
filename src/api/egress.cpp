#include <pichi/api/egress.hpp>

using namespace std;

namespace pichi::api {

void Egress::update(string const& name, OutboundVO vo) { c_[name] = move(vo); }

void Egress::erase(string_view name) { c_.erase(c_.find(name)); }

Egress::ConstIterator Egress::begin() const noexcept { return cbegin(c_); }

Egress::ConstIterator Egress::end() const noexcept { return cend(c_); }

Egress::ConstIterator Egress::find(string_view name) const { return c_.find(name); }

} // namespace pichi::api