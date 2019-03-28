#include <pichi/api/ingress_manager.hpp>
#include <pichi/asserts.hpp>

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
namespace sys = boost::system;
using ip::tcp;

namespace pichi::api {

IngressManager::IngressManager(boost::asio::io_context& io, Handler onChange)
  : io_{io}, onChange_{onChange}, c_{}
{
}

IngressManager::ValueType IngressManager::generatePair(DelegateIterator it)
{
  return make_pair(cref(it->first), cref(it->second.first));
}

IngressManager::ConstIterator IngressManager::begin() const noexcept
{
  return {cbegin(c_), cend(c_), &IngressManager::generatePair};
}

IngressManager::ConstIterator IngressManager::end() const noexcept
{
  return {cend(c_), cend(c_), &IngressManager::generatePair};
}

void IngressManager::update(string const& name, IngressVO ivo)
{
  assertFalse(ivo.type_ == AdapterType::DIRECT, PichiError::MISC);
  assertFalse(ivo.type_ == AdapterType::REJECT, PichiError::MISC);
  auto it = c_.find(name);
  if (it == std::end(c_)) {
    auto p = c_.try_emplace(
        name, make_pair(move(ivo), Acceptor{io_, {ip::make_address(ivo.bind_), ivo.port_}}));
    assertTrue(p.second, PichiError::MISC);
    it = p.first;
  }
  else {
    it->second = make_pair(move(ivo), Acceptor{io_, {ip::make_address(ivo.bind_), ivo.port_}});
  }
  auto&& [iname, v] = *it;
  auto&& [vo, acceptor] = v;
  invoke(onChange_, acceptor, iname, vo);
}

void IngressManager::erase(string_view name)
{
  auto it = c_.find(name);
  if (it != std::end(c_)) c_.erase(it);
}

} // namespace pichi::api
