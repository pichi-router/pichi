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

bool Geo::match(string_view address, string_view country) const
{
  auto ec = 0;
  auto status = MMDB_SUCCESS;
  auto result = MMDB_lookup_string(db_.get(), address.data(), &ec, &status);
  assertTrue(ec == 0, PichiError::MISC, MMDB_strerror(ec));
  assertTrue(status == MMDB_SUCCESS, PichiError::MISC, MMDB_strerror(ec));

  if (!result.found_entry) return false;

  auto entry = MMDB_entry_data_s{};
  status = MMDB_get_value(&result.entry, &entry, "country", "iso_code", nullptr);
  assertTrue(status == MMDB_SUCCESS, PichiError::MISC, MMDB_strerror(ec));

  if (!entry.has_data) return false;
  assertTrue(entry.type == MMDB_DATA_TYPE_UTF8_STRING, PichiError::MISC);

  return string_view{entry.utf8_string, entry.data_size} == country;
}

Router::ValueType Router::generatePair(DelegateIterator it)
{
  return make_pair(ref(it->first), ref(it->second.first));
}

Router::Router(char const* fn) : geo_{fn} {}

string_view Router::route(net::Endpoint const& e, ResolvedResult const& r, string_view inbound,
                          AdapterType type) const
{
  auto it = find_if(
      cbegin(order_), cend(order_), [& rules = as_const(rules_), &e, &r, inbound, type](auto name) {
        auto it = rules.find(name);
        assertFalse(it == cend(rules), PichiError::MISC);
        auto& matchers = as_const(it->second.second);
        return any_of(cbegin(matchers), cend(matchers), [&e, &r, inbound, type](auto&& matcher) {
          return matcher(e, r, inbound, type);
        });
      });
  auto rule = it != cend(order_) ? *it : "DEFAUTL rule"sv;
  auto outbound =
      string_view{it != cend(order_) ? rules_.find(*it)->second.first.outbound_ : default_};
  cout << e.host_ << ":" << e.port_ << " -> " << outbound << " (" << rule << ")\n";
  return outbound;
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
  transform(cbegin(vo.inbound_), cend(vo.inbound_), back_inserter(matchers), [](auto&& i) {
    return [&i](auto&&, auto&&, auto inbound, auto) { return i == inbound; };
  });
  transform(cbegin(vo.type_), cend(vo.type_), back_inserter(matchers), [](auto t) {
    // Inbound type shouldn't be DIRECT or REJECT
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
                  return geo.match(entry.endpoint().address().to_string(), country);
                });
              };
            });
  guard.disable();
}

void Router::erase(string_view name)
{
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
  if (rvo.default_) default_ = rvo.default_.value();
  order_ = move(tmp);
}

} // namespace pichi::api
