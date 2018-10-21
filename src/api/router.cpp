#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <maxminddb.h>
#include <pichi/api/router.hpp>
#include <pichi/asserts.hpp>
#include <pichi/scope_guard.hpp>
#include <regex>

using namespace std;
namespace ip = boost::asio::ip;
namespace sys = boost::system;
using ip::tcp;

namespace pichi::api {

bool matchPattern(string_view remote, string_view pattern)
{
  return regex_search(cbegin(remote), cend(remote), regex{cbegin(pattern), cend(pattern)});
}

bool matchDomain(string_view subdomain, string_view domain)
{
  // TODO domain can start with '.'
  assertFalse(!domain.empty() && domain[0] == '.', PichiError::MISC);
  assertFalse(!subdomain.empty() && subdomain[0] == '.', PichiError::MISC);
  return !domain.empty() && !subdomain.empty() && // not matching if anyone is empty
         (subdomain == domain ||                  // same
          (subdomain.size() > domain.size() &&    // subdomain can not be shorter than domain
           equal(crbegin(domain), crend(domain),
                 crbegin(subdomain)) && // subdomain ends up with domain
           subdomain[subdomain.size() - domain.size() - 1] == '.'));
}

Geo::Geo(char const* fn) : db_{make_unique<MMDB_s>()}
{
  auto status = MMDB_open(fn, MMDB_MODE_MMAP, db_.get());
  assertTrue(status == MMDB_SUCCESS, PichiError::MISC, MMDB_strerror(status));
}

Geo::~Geo() { MMDB_close(db_.get()); }

bool Geo::match(tcp::endpoint const& endpoint, string_view country) const
{
  auto status = MMDB_SUCCESS;
  auto result = MMDB_lookup_sockaddr(db_.get(), endpoint.data(), &status);

  // TODO log it
  if (status != MMDB_SUCCESS || !result.found_entry) return false;

  auto entry = MMDB_entry_data_s{};
  status = MMDB_get_value(&result.entry, &entry, "country", "iso_code", nullptr);

  // TODO log it
  if (status != MMDB_SUCCESS || !entry.has_data) return false;
  assertTrue(entry.type == MMDB_DATA_TYPE_UTF8_STRING, PichiError::MISC);

  return string_view{entry.utf8_string, entry.data_size} == country;
}

Router::ValueType Router::generatePair(DelegateIterator it)
{
  return make_pair(ref(it->first), ref(it->second.first));
}

Router::Router(char const* fn) : geo_{fn} {}

string_view Router::route(net::Endpoint const& e, string_view ingress, AdapterType type,
                          function<ResolvedResult()> const& resolve) const
{
  auto r = ResolvedResult{};
  auto resolved = false;
  auto it = find_if(cbegin(order_), cend(order_), [&, this](auto name) {
    auto it = rules_.find(name);
    assertFalse(it == cend(rules_), PichiError::MISC);
    auto& rule = as_const(it->second.first);
    auto& matchers = as_const(it->second.second);
    auto needResolving = !rule.range_.empty() || !rule.country_.empty();
    if (!resolved && needResolving) {
      r = resolve();
      resolved = true;
    }
    return any_of(cbegin(matchers), cend(matchers),
                  [&](auto&& matcher) { return matcher(e, r, ingress, type); });
  });
  auto rule = it != cend(order_) ? *it : "DEFAULT rule"sv;
  auto egress = string_view{it != cend(order_) ? rules_.find(*it)->second.first.egress_ : default_};
  cout << e.host_ << ":" << e.port_ << " -> " << egress << " (" << rule << ")" << endl;
  return egress;
}

void Router::update(string const& name, RuleVO rvo)
{
  rules_[name] = make_pair(move(rvo), vector<Matcher>{});
  auto it = rules_.find(name);
  auto guard = makeScopeGuard([it, this]() { rules_.erase(it); });
  auto& vo = as_const(it->second.first);
  auto& matchers = it->second.second;

  transform(cbegin(vo.range_), cend(vo.range_), back_inserter(matchers),
            [](auto&& range) -> Matcher {
              auto ec = sys::error_code{};
              auto n4 = ip::make_network_v4(range, ec);
              if (ec)
                return [n6 = ip::make_network_v6(range)](auto&&, auto&& r, auto, auto) {
                  return any_of(cbegin(r), cend(r), [n6](auto&& entry) {
                    auto address = entry.endpoint().address();
                    return address.is_v6() && n6.hosts().find(address.to_v6()) != cend(n6.hosts());
                  });
                };
              else
                return [n4](auto&&, auto&& r, auto, auto) {
                  return any_of(cbegin(r), cend(r), [n4](auto&& entry) {
                    auto address = entry.endpoint().address();
                    return address.is_v4() && n4.hosts().find(address.to_v4()) != cend(n4.hosts());
                  });
                };
            });
  transform(cbegin(vo.ingress_), cend(vo.ingress_), back_inserter(matchers), [](auto&& i) {
    return [&i](auto&&, auto&&, auto ingress, auto) { return i == ingress; };
  });
  transform(cbegin(vo.type_), cend(vo.type_), back_inserter(matchers), [](auto t) {
    // ingress type shouldn't be DIRECT or REJECT
    assertFalse(t == AdapterType::DIRECT, PichiError::MISC);
    assertFalse(t == AdapterType::REJECT, PichiError::MISC);
    return [t](auto&&, auto&&, auto, auto type) { return t == type; };
  });
  transform(cbegin(vo.pattern_), cend(vo.pattern_), back_inserter(matchers), [](auto&& pattern) {
    return [&pattern](auto&& e, auto&&, auto, auto) { return matchPattern(e.host_, pattern); };
  });
  transform(cbegin(vo.domain_), cend(vo.domain_), back_inserter(matchers), [](auto&& domain) {
    return [&domain](auto&& e, auto&&, auto, auto) {
      return e.type_ == net::Endpoint::Type::DOMAIN_NAME && matchDomain(e.host_, domain);
    };
  });
  transform(cbegin(vo.country_), cend(vo.country_), back_inserter(matchers),
            [& geo = as_const(geo_)](auto&& country) {
              return [&country, &geo](auto&&, auto&& r, auto, auto) {
                return any_of(cbegin(r), cend(r), [&geo, &country](auto&& entry) {
                  return geo.match(entry.endpoint(), country);
                });
              };
            });
  guard.disable();
}

void Router::erase(string_view name)
{
  assertTrue(find(cbegin(order_), cend(order_), name) == cend(order_), PichiError::RES_IN_USE);
  auto it = rules_.find(name);
  if (it != std::end(rules_)) rules_.erase(it);
}

Router::ConstIterator Router::begin() const noexcept
{
  return {cbegin(rules_), cend(rules_), &Router::generatePair};
}

Router::ConstIterator Router::end() const noexcept
{
  return {cend(rules_), cend(rules_), &Router::generatePair};
}

bool Router::isUsed(string_view egress) const
{
  return default_ == egress || rules_.find(egress) != cend(rules_);
}

RouteVO Router::getRoute() const
{
  auto rvo = RouteVO{};
  rvo.default_.emplace(default_);
  transform(cbegin(order_), cend(order_), back_inserter(rvo.rules_), [](auto rule) {
    return string{rule.data(), rule.size()};
  });
  return rvo;
}

void Router::setRoute(RouteVO const& rvo)
{
  auto& rules = as_const(rules_);
  auto tmp = vector<string_view>{};
  transform(cbegin(rvo.rules_), cend(rvo.rules_), back_inserter(tmp), [&rules](auto&& r) {
    auto it = rules.find(r);
    assertFalse(it == cend(rules), PichiError::MISC);
    return string_view{it->first};
  });
  if (rvo.default_) default_ = *rvo.default_;
  order_ = move(tmp);
}

} // namespace pichi::api
