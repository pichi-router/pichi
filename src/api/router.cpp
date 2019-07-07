#include <pichi/config.hpp>

#ifdef NO_RETURN_STD_MOVE_FOR_BOOST_ASIO
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-std-move"
#endif // __clang__
#include <boost/asio/ip/basic_resolver.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__
#endif // NO_RETURN_STD_MOVE_FOR_BOOST_ASIO

#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <maxminddb.h>
#include <numeric>
#include <pichi/api/router.hpp>
#include <pichi/asserts.hpp>
#include <pichi/scope_guard.hpp>
#include <regex>

using namespace std;
namespace ip = boost::asio::ip;
namespace sys = boost::system;
using ip::tcp;

namespace pichi::api {

namespace msg {

static auto const DM_INVALID = "Invalid domain string"sv;
static auto const RG_INVALID = "Invalid IP range string"sv;
static auto const AT_INVALID = "Invalid adapter type string"sv;

} // namespace msg

bool matchPattern(string_view remote, string_view pattern)
{
  return regex_search(cbegin(remote), cend(remote), regex{cbegin(pattern), cend(pattern)});
}

bool matchDomain(string_view subdomain, string_view domain)
{
  domain.remove_prefix(min(domain.size(), domain.find_first_not_of('.')));
  assertFalse(domain.empty(), PichiError::SEMANTIC_ERROR, msg::DM_INVALID);
  assertFalse(subdomain.empty(), msg::DM_INVALID);
  assertFalse('.' == subdomain.front(), msg::DM_INVALID);
  return subdomain == domain ||               // same
         (subdomain.size() > domain.size() && // subdomain can not be shorter than domain
          equal(crbegin(domain), crend(domain),
                crbegin(subdomain)) && // subdomain ends up with domain
          subdomain[subdomain.size() - domain.size() - 1] == '.');
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
                          ResolvedResult const& r) const
{
  auto rule = "DEFAUTL rule"sv;
  auto it = find_if(cbegin(route_.rules_), cend(route_.rules_), [&, this](auto&& rules) {
    return any_of(cbegin(rules.first), cend(rules.first), [&, this](auto&& name) {
      auto it = rules_.find(name);
      assertFalse(it == cend(rules_));
      auto& matchers = as_const(it->second.second);
      auto b = any_of(cbegin(matchers), cend(matchers),
                      [&](auto&& matcher) { return matcher(e, r, ingress, type); });
      if (b) rule = name;
      return b;
    });
  });
  auto egress = string_view{it != cend(route_.rules_) ? it->second : *route_.default_};
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
              if (ec) {
                auto n6 = ip::make_network_v6(range, ec);
                assertFalse(static_cast<bool>(ec), PichiError::SEMANTIC_ERROR, msg::RG_INVALID);
                return [n6](auto&&, auto&& r, auto, auto) {
                  return any_of(cbegin(r), cend(r), [n6](auto&& entry) {
                    auto address = entry.endpoint().address();
                    return address.is_v6() && n6.hosts().find(address.to_v6()) != cend(n6.hosts());
                  });
                };
              }
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
    assertFalse(t == AdapterType::DIRECT, PichiError::SEMANTIC_ERROR, msg::AT_INVALID);
    assertFalse(t == AdapterType::REJECT, PichiError::SEMANTIC_ERROR, msg::AT_INVALID);
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
  assertFalse(any_of(cbegin(route_.rules_), cend(route_.rules_),
                     [name](auto&& items) {
                       return any_of(cbegin(items.first), cend(items.first),
                                     [name](auto&& rule) { return rule == name; });
                     }),
              PichiError::RES_IN_USE);
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
  return route_.default_ == egress || any_of(cbegin(route_.rules_), cend(route_.rules_),
                                             [=](auto&& item) { return item.second == egress; });
}

bool Router::needResloving() const { return needResolving_; }

RouteVO Router::getRoute() const { return route_; }

void Router::setRoute(RouteVO rvo)
{
  needResolving_ = accumulate(
      cbegin(rvo.rules_), cend(rvo.rules_), false, [this](auto needResolving, auto&& item) {
        return accumulate(cbegin(item.first), cend(item.first), needResolving,
                          [this](auto needResolving, auto&& name) {
                            auto it = rules_.find(name);
                            assertFalse(it == cend(rules_), PichiError::SEMANTIC_ERROR,
                                        "Unknown rules"sv);
                            auto& rule = it->second.first;
                            return needResolving || !(rule.range_.empty() && rule.country_.empty());
                          });
      });
  route_.rules_ = move(rvo.rules_);
  if (rvo.default_.has_value()) route_.default_ = rvo.default_;
}

} // namespace pichi::api
