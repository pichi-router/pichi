#include "config.h"
#include <boost/asio/system_timer.hpp>
#include <chrono>
#include <iostream>
#include <pichi/api/egress_manager.hpp>
#include <pichi/api/ingress_manager.hpp>
#include <pichi/api/router.hpp>
#include <pichi/api/session.hpp>
#include <pichi/asserts.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/spawn.hpp>

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
namespace sys = boost::system;
using ip::tcp;

namespace pichi::api {

static auto const RANDOM_EJECTOR = EgressVO{AdapterType::REJECT, {}, {}, {}, {}, DelayMode::RANDOM};
static auto const IV_EXPIRE_TIME = 1h;

IngressManager::IngressManager(Strand strand, Router const& router, EgressManager const& eManager)
  : strand_{strand}, router_{router}, eManager_{eManager}, c_{}
{
}

IngressManager::ValueType IngressManager::generatePair(DelegateIterator it)
{
  return make_pair(cref(it->first), cref(it->second.first));
}

template <typename Yield> bool IngressManager::isDuplicated(ConstBuffer<uint8_t> raw, Yield yield)
{
  if (raw.size() == 0) return false;

  auto [it, inserted] = ivs_.insert({raw.cbegin(), raw.cend()});
  if (!inserted) {
    cout << "Pichi Error: Duplicated IV" << endl;
    return true;
  }
  net::spawn(strand_, [it = it, this](auto yield) {
    // Exceptions prohibited
    auto ec = sys::error_code{};
    asio::system_timer{strand_.context(), IV_EXPIRE_TIME}.async_wait(yield[ec]);
    ivs_.erase(it);
  });
  return false;
}

template <typename Yield>
pair<EgressVO const&, net::Endpoint>
IngressManager::route(net::Endpoint const& remote, string_view iname, AdapterType type, Yield yield)
{
  auto resolve = [&yield, &remote, &io = strand_.context()]() {
    auto ec = sys::error_code{};
    auto r = tcp::resolver{io}.async_resolve(remote.host_, remote.port_, yield[ec]);
    return ec ? tcp::resolver::results_type{} : r;
  };
  auto it = eManager_.find(router_.route(remote, iname, type, resolve));
  assertFalse(it == cend(eManager_));
  return make_pair(cref(it->second), remote);
}

IngressManager::ConstIterator IngressManager::begin() const noexcept
{
  return {cbegin(c_), cend(c_), &IngressManager::generatePair};
}

IngressManager::ConstIterator IngressManager::end() const noexcept
{
  return {cend(c_), cend(c_), &IngressManager::generatePair};
}

template <typename Yield> void IngressManager::listen(typename Container::iterator it, Yield yield)
{
  auto& acceptor = it->second.second;

  while (true) {
    net::spawn(strand_, [s = acceptor.async_accept(yield), &vo = as_const(it->second.first),
                         &iname = as_const(it->first), this](auto yield) mutable {
      auto& io = strand_.context();
      auto ingress = net::makeIngress(vo, move(s));
      auto iv = array<uint8_t, 32>{};
      auto&& [evo, remote] = isDuplicated({iv, ingress->readIV(iv, yield)}, yield) ?
                                 make_pair(cref(RANDOM_EJECTOR), net::Endpoint{}) :
                                 route(ingress->readRemote(yield), iname, vo.type_, yield);
      auto next =
          evo.type_ == AdapterType::REJECT || evo.type_ == AdapterType::DIRECT ?
              remote :
              net::Endpoint{net::detectHostType(*evo.host_), *evo.host_, to_string(*evo.port_)};
      make_shared<Session>(io, move(ingress), net::makeEgress(evo, tcp::socket{io}))
          ->start(remote, next);
    });
  }
}

void IngressManager::update(string const& name, IngressVO ivo)
{
  assertFalse(ivo.type_ == AdapterType::DIRECT, PichiError::MISC);
  assertFalse(ivo.type_ == AdapterType::REJECT, PichiError::MISC);
#ifndef ENABLE_TLS
  assertFalse(ivo.tls_.has_value() && *ivo.tls_, PichiError::SEMANTIC_ERROR, "TLS not supported");
#endif // ENABLE_TLS

  auto it = c_.find(name);
  if (it == std::end(c_)) {
    auto p = c_.try_emplace(
        name, make_pair(move(ivo),
                        Acceptor{strand_.context(), {ip::make_address(ivo.bind_), ivo.port_}}));
    assertTrue(p.second, PichiError::MISC);
    it = p.first;
  }
  else {
    it->second =
        make_pair(move(ivo), Acceptor{strand_.context(), {ip::make_address(ivo.bind_), ivo.port_}});
  }
  net::spawn(
      strand_, [it, this](auto yield) { listen(it, yield); },
      [ it, this ](auto eptr, auto) noexcept {
        try {
          if (eptr) rethrow_exception(eptr);
        }
        catch (sys::system_error const& e) {
          if (e.code() != asio::error::operation_aborted) {
            assert(it != std::end(c_));
            c_.erase(it);
          }
        }
      });
}

void IngressManager::erase(string_view name)
{
  auto it = c_.find(name);
  if (it != std::end(c_)) c_.erase(it);
}

} // namespace pichi::api
