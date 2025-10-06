#include "pichi/common/config.hpp"
#include <algorithm>
#include <boost/asio/ip/tcp.hpp>
#include <cctype>
#include <cstdint>
#include <maxminddb.h>
#include <pichi/actor/router.hpp>
#include <pichi/common/adapter.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/common/mmdb.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/rule.hpp>
#include <pichi/vo/to_json.hpp>
#include <ranges>
#include <regex>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::literals;

namespace asio  = boost::asio;
namespace ip    = asio::ip;
namespace json  = rapidjson;
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

template <typename Results> bool Matcher::match_range(Endpoint const&, Results&& rs) const
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

Matcher::Matcher(std::string_view name, vo::Rule const& rule) : name_{name}
{
  rngs::for_each(rule.range_, [this](auto&& range) {
    auto ec = sys::error_code{};
    auto n4 = ip::make_network_v4(range, ec);
    if (ec) {
      auto n6 = ip::make_network_v6(range, ec);
      assertTrue(!ec, PichiError::SEMANTIC_ERROR);
      ranges6_.push_back(n6);
    }
    else
      ranges4_.push_back(n4);
  });

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
}

bool Matcher::need_resolving() const
{
  return !ranges4_.empty() || !ranges6_.empty() || !countries_.empty();
}

std::string_view Matcher::name() const { return name_; }

template <typename Results>
bool Matcher::match(Endpoint const& peer, std::string_view iname, AdapterType type, Results&& rs)
    const
{
  return match_range(peer, rs) || inames_.contains(iname) || types_.contains(type) ||
         match_pattern(peer.host_) || match_domain(peer) || rngs::any_of(rs, [this](auto&& r) {
           return rngs::any_of(countries_, [&](auto&& country) {
             return g_mmdb.has_value() && g_mmdb->match(r.endpoint().data(), country);
           });
         });
}

}  // namespace detail

static auto const DEFAULT_EGRESS_NAME = "direct"s;

template <rngs::range Range> std::string to_string(Range const& range)
{
  auto alloc = json::Document::AllocatorType{};
  return vo::toString(vo::toJson(rngs::cbegin(range), rngs::cend(range), alloc));
}

void Router::build_matchers()
{
  auto matchers = Matchers{};
  detail::insert_range(
      matchers,
      route_.rules_ | views::transform([this](auto&& p) {
        assertTrue(egresses_.contains(p.second), PichiError::SEMANTIC_ERROR);
        auto ms = std::vector<detail::Matcher>{};
        detail::insert_range(ms, p.first | views::transform([this](auto&& rname) {
                                   assertTrue(rules_.contains(rname), PichiError::SEMANTIC_ERROR);
                                   return detail::Matcher{rname, rules_[rname]};
                                 }));
        return std::make_pair(std::string_view{p.second}, std::move(ms));
      })
  );
  matchers_ = std::move(matchers);
}

Router::Router(IOExecutor ex) :
  ex_{std::move(ex)},
  egresses_{{DEFAULT_EGRESS_NAME, vo::Egress{.type_ = AdapterType::DIRECT}}},
  route_{.default_ = DEFAULT_EGRESS_NAME}
{
}

std::string Router::get_egresses() const { return to_string(egresses_); }

Router::Pointer Router::del_egress(std::string const& name)
{
  assertFalse(matchers_.contains(name), PichiError::RES_IN_USE);
  auto rt = std::make_shared<Router>(*this);
  rt->egresses_.erase(name);
  return rt;
}

Router::Pointer Router::put_egress(std::string const& name, std::string_view value)
{
  auto vo = vo::parse<vo::Egress>(value);
  auto rt = std::make_shared<Router>(*this);

  rt->egresses_[name] = std::move(vo);
  return rt;
}

std::string Router::get_rules() const { return to_string(rules_); }

Router::Pointer Router::del_rule(std::string const& name)
{
  assertTrue(
      rngs::none_of(
          matchers_,
          [&](auto&& p) {
            return rngs::none_of(p.second, [&](auto&& matcher) { return matcher.name() == name; });
          }
      ),
      PichiError::RES_IN_USE
  );
  auto rt = std::make_shared<Router>(*this);
  rt->rules_.erase(name);
  return rt;
}

Router::Pointer Router::put_rule(std::string const& name, std::string_view value)
{
  auto vo = vo::parse<vo::Rule>(value);
  auto rt = std::make_shared<Router>(*this);

  rt->rules_[name] = std::move(vo);
  rt->build_matchers();
  return rt;
}

std::string Router::get_route() const
{
  auto alloc = json::Document::AllocatorType{};
  return vo::toString(vo::toJson(route_, alloc));
}

Router::Pointer Router::put_route(std::string_view value)
{
  auto vo = vo::parse<vo::Route>(value);
  if (!vo.default_.has_value()) vo.default_ = route_.default_;

  auto rt = std::make_shared<Router>(*this);

  rt->route_ = std::move(vo);
  rt->build_matchers();
  return rt;
}

Awaitable<std::tuple<std::string, std::string, vo::Egress>>
    Router::route(Endpoint const& peer, std::string_view iname, AdapterType itype) const
{
  auto rs = ResolveResults{};
  for (auto&& [ename, ms] : matchers_) {
    if (rs.empty() && rngs::any_of(ms, [](auto&& matcher) { return matcher.need_resolving(); })) {
      auto r = ip::tcp::resolver{ex_};
      rs = co_await r.async_resolve(peer.host_, std::to_string(peer.port_), asio::use_awaitable);
    }
    auto it =
        rngs::find_if(ms, [&](auto&& matcher) { return matcher.match(peer, iname, itype, rs); });
    if (it != rngs::end(ms))
      co_return std::make_tuple(it->name(), ename, egresses_.at(std::to_string(ename)));
  }
  co_return std::make_tuple("*"sv, *route_.default_, egresses_.at(*route_.default_));
}

}  // namespace pichi::actor
