#ifndef PICHI_ACTOR_ROUTER_HPP
#define PICHI_ACTOR_ROUTER_HPP

#include <boost/asio/ip/basic_resolver_results.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include <optional>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/service/mmdb.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/rule.hpp>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace boost::asio::ip {

class tcp;
template <typename InternetProtocol> class basic_resolver_results;

}  // namespace boost::asio::ip

namespace pichi::actor {

using ResolveResults = boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>;

namespace detail {

class Matcher {
private:
  bool match_range(ResolveResults const&) const;

  bool match_pattern(std::string const&) const;

  bool match_domain(Endpoint const&) const;

public:
  Matcher(std::string_view, vo::Rule const&, std::string_view, vo::Egress const&);
  Matcher(
      std::string_view, vo::Rule const&, std::string_view, vo::Egress const&,
      std::function<Awaitable<bool>(Endpoint const&)> f
  );

  bool match(
      Endpoint const&, std::string const&, AdapterType, std::optional<ResolveResults> const&,
      service::Mmdb& mmdb
  ) const;

  bool need_resolving() const;

  std::string const& rname() const;
  std::string const& ename() const;
  vo::Egress const&  egress() const;

private:
  std::string rname_;
  std::string ename_;
  vo::Egress  egress_;

  bool resolve_;

  std::vector<boost::asio::ip::network_v4> ranges4_ = {};
  std::vector<boost::asio::ip::network_v6> ranges6_ = {};

  std::unordered_set<std::string> inames_ = {};

  std::unordered_set<AdapterType> types_ = {};

  std::vector<std::regex> patterns_ = {};

  std::vector<std::string> domains_   = {};
  std::vector<std::string> countries_ = {};
};

template <typename Value> using ValueMap = std::unordered_map<std::string, Value>;

}  // namespace detail

class Router {
private:
  template <typename Value> using ValueMap = detail::ValueMap<Value>;

  using Matchers = std::vector<detail::Matcher>;

public:
  Router(
      IOExecutor const&, ValueMap<vo::Egress> const&, ValueMap<vo::Rule> const&, vo::Route const&
  );

  Awaitable<std::tuple<std::string, std::string, vo::Egress>>
      route(Endpoint const&, std::string const&, AdapterType) const;

  Awaitable<std::tuple<std::string, std::string, vo::Egress>>
      route(Endpoint const&, std::string const&, AdapterType, Awaitable<ResolveResults>) const;

private:
  IOExecutor ex_;
  Matchers   matchers_ = {};

  std::tuple<std::string, std::string, vo::Egress> default_;
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_ROUTER_HPP
