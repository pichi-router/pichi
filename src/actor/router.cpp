#include "pichi/common/config.hpp"
#include <algorithm>
#include <boost/asio/execution/context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>
#include <cctype>
#include <cstdint>
#include <maxminddb.h>
#include <numeric>
#include <pichi/actor/router.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/parse.hpp>
#include <ranges>
#include <utility>

using namespace std::literals;

namespace asio  = boost::asio;
namespace ip    = asio::ip;
namespace rngs  = std::ranges;
namespace sys   = boost::system;
namespace views = rngs::views;

namespace pichi::actor {

namespace detail {

template <typename C, rngs::range R> auto insert_range(C& c, R&& r)
{
  if constexpr (requires(C c) { c.insert(rngs::begin(r), rngs::end(r)); }) {
    return c.insert(rngs::begin(r), rngs::end(r));
  }
  else {
    return c.insert(rngs::end(c), rngs::begin(r), rngs::end(r));
  }
}

template <rngs::range Range> auto to_lower_str(Range&& orig)
{
  auto v = orig | views::transform([](uint8_t c) { return std::tolower(c); });
  return std::string{rngs::cbegin(v), rngs::cend(v)};
}

bool Matcher::match_range(ResolveResults const& rs) const
{
  static auto match = [](auto const& ranges, auto const& addr) {
    return rngs::any_of(ranges, [&](auto net) {
      auto hosts = net.hosts();
      return hosts.find(addr) != hosts.end();
    });
  };
  return rngs::any_of(rs, [this](auto&& entry) {
    auto addr = entry.endpoint().address();
    return (addr.is_v4() && match(ranges4_, addr.to_v4())) ||
           (addr.is_v6() && match(ranges6_, addr.to_v6()));
  });
}

bool Matcher::match_pattern(std::string const& peer) const
{
  return rngs::any_of(patterns_, [&](auto&& pattern) { return std::regex_search(peer, pattern); });
}

bool Matcher::match_domain(Endpoint const& peer) const
{
  return peer.type_ == EndpointType::DOMAIN_NAME && !peer.host_.empty() &&
         rngs::any_of(domains_, [sub = to_lower_str(peer.host_)](auto&& domain) {
           auto d = domain | views::reverse;
           auto s =
               (sub.front() == '.' ? views::drop(sub, 1) : views::drop(sub, 0)) | views::reverse;

           auto dot = s | views::drop(rngs::size(d)) | views::take(1);
           return rngs::equal(d, s | views::take(rngs::size(d))) &&
                  (rngs::empty(dot) || *rngs::cbegin(dot) == '.');
         });
}

Matcher::Matcher(
    std::string_view rname, vo::Rule const& rule, std::string_view ename, vo::Egress const& egress
)
  : rname_{rname},
    ename_{ename},
    egress_{egress},
    resolve_{!rngs::empty(rule.country_) || !rngs::empty(rule.range_)}
{
  // FIXME Utilize std::views::concat if being able to apply C++26
  auto append = [this](auto&& range) {
    auto ec = sys::error_code{};
    auto n4 = ip::make_network_v4(range, ec);
    if (ec) {
      auto n6 = ip::make_network_v6(range, ec);
      assertTrue(!ec, PichiError::SEMANTIC_ERROR);
      ranges6_.push_back(n6);
    }
    else
      ranges4_.push_back(n4);
  };

  rngs::for_each(rule.range_, append);
  rngs::for_each(rule.range_nr_, append);

  insert_range(inames_, rule.ingress_);
  insert_range(types_, rule.type_);

  insert_range(patterns_, rule.pattern_ | views::transform([](auto&& s) { return std::regex{s}; }));

  insert_range(domains_, rule.domain_ | views::transform([](auto&& s) {
                           return rngs::empty(s) || *rngs::cbegin(s) != '.' ? views::drop(s, 0)
                                                                            : views::drop(s, 1);
                         }) | views::filter([](auto&& v) {
                           return !rngs::empty(v);
                         }) | views::transform([](auto&& s) { return to_lower_str(s); }));

  insert_range(countries_, rule.country_);
  insert_range(countries_, rule.country_nr_);
}

bool Matcher::need_resolving() const { return resolve_; }

std::string const& Matcher::rname() const { return rname_; }

std::string const& Matcher::ename() const { return ename_; }

vo::Egress const& Matcher::egress() const { return egress_; }

bool Matcher::match(
    Endpoint const& peer, std::string const& iname, AdapterType type,
    std::optional<ResolveResults> const& rs, service::Mmdb& mmdb
) const
{
  return (rs.has_value() && !rs->empty() &&
          (match_range(*rs) || rngs::any_of(
                                   *rs,
                                   [&, this](auto&& r) {
                                     return rngs::any_of(countries_, [&](auto&& country) {
                                       return mmdb.match(r.endpoint().data(), country);
                                     });
                                   }
                               ))) ||
         inames_.contains(iname) || types_.contains(type) || match_pattern(peer.host_) ||
         match_domain(peer);
}

static auto parse_route(
    vo::Route const& route, ValueMap<vo::Egress> const& egresses, ValueMap<vo::Rule> const& rules
)
{
  auto ret = std::vector<Matcher>{};
  ret.reserve(
      std::accumulate(
          rngs::cbegin(route.rules_),
          rngs::cend(route.rules_),
          0_sz,
          [](auto s, auto&& p) { return s + rngs::size(p.first); }
      )
  );
  for (auto&& p : route.rules_) {
    for (auto&& rname : p.first) {
      assertTrue(rules.contains(rname));
      assertTrue(egresses.contains(p.second));
      ret.emplace_back(rname, rules.at(rname), p.second, egresses.at(p.second));
    }
  }
  return ret;
}

}  // namespace detail

Router::Router(
    IOExecutor const& ex, ValueMap<vo::Egress> const& egresses, ValueMap<vo::Rule> const& rules,
    vo::Route const& route
)
  : ex_{ex},
    matchers_{detail::parse_route(route, egresses, rules)},
    default_{std::make_tuple("*"s, *route.default_, egresses.at(*route.default_))}
{
}

Awaitable<std::tuple<std::string, std::string, vo::Egress>>
    Router::route(Endpoint const& peer, std::string const& iname, AdapterType itype) const
{
  auto r = ip::tcp::resolver{ex_};
  co_return co_await route(
      peer,
      iname,
      itype,
      std::invoke(
          [&](auto&& peer) {
            return r.async_resolve(peer.host_, std::to_string(peer.port_), asio::use_awaitable);
          },
          peer
      )
  );
}

Awaitable<std::tuple<std::string, std::string, vo::Egress>> Router::route(
    Endpoint const& peer, std::string const& iname, AdapterType itype,
    Awaitable<ResolveResults> resolve
) const
{
  auto& mmdb = asio::use_service<service::Mmdb>(asio::query(ex_, asio::execution::context));

  auto ec = sys::error_code{};
  auto rs = std::optional<ResolveResults>{};
  if (peer.type_ != EndpointType::DOMAIN_NAME)
    // FIXME ResolveResults::create is NOT a public and well-documented API
    rs.emplace(
        ResolveResults::create(ip::tcp::endpoint{ip::make_address(peer.host_), peer.port_}, "", "")
    );

  for (auto&& matcher : matchers_) {
    if (!rs.has_value() && matcher.need_resolving()) {
      rs = co_await redirect(std::move(resolve), ec);

      // For test purpose
      if (ec == PichiError::MISC) asio::detail::throw_error(ec);

      // Skip further resolving
      if (!rs.has_value()) rs = ResolveResults{};
    }
    if (matcher.match(peer, iname, itype, rs, mmdb))
      co_return std::make_tuple(matcher.rname(), matcher.ename(), matcher.egress());
  }
  co_return default_;
}

}  // namespace pichi::actor
