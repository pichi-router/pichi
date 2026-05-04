#ifndef PICHI_ACTOR_ROUTER_HPP
#define PICHI_ACTOR_ROUTER_HPP

#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include <optional>
#include <pichi/common/adapter.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/mmdb.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/rule.hpp>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pichi::actor {

namespace detail {

class Matcher {
private:
  bool match_range(Endpoint const&, ResolveResults const&) const;

  bool match_pattern(std::string const&) const;

  bool match_domain(Endpoint const&) const;

  Awaitable<bool> match_range(Endpoint const&) const;

public:
  Matcher(std::string_view, vo::Rule const&, std::string_view, vo::Egress const&);
  Matcher(
      std::string_view, vo::Rule const&, std::string_view, vo::Egress const&,
      std::function<Awaitable<bool>(Endpoint const&)> f
  );

  bool match(
      Endpoint const&, std::string const&, AdapterType, std::optional<ResolveResults> const&,
      Mmdb& mmdb
  ) const;

  Awaitable<bool> match() const;

  bool need_resolving() const;

  std::string const& rname() const;
  std::string const& ename() const;
  vo::Egress const&  egress() const;

private:
  std::string rname_;
  std::string ename_;
  vo::Egress  egress_;

  std::vector<boost::asio::ip::network_v4> ranges4_ = {};
  std::vector<boost::asio::ip::network_v6> ranges6_ = {};

  std::unordered_set<std::string> inames_ = {};

  std::unordered_set<AdapterType> types_ = {};

  std::vector<std::regex> patterns_ = {};

  std::vector<std::string> domains_   = {};
  std::vector<std::string> countries_ = {};
};

}  // namespace detail

class Router {
private:
  using Matchers = std::vector<detail::Matcher>;

public:
  Router(
      IOExecutor, std::unordered_map<std::string, vo::Egress> const&,
      std::unordered_map<std::string, vo::Rule> const&, vo::Route const&
  );

  Awaitable<std::tuple<std::string, std::string, vo::Egress>> route(
      Endpoint const&, std::string_view, AdapterType, std::optional<ResolveResults> = std::nullopt
  ) const;

private:
  IOExecutor ex_;
  Matchers   matchers_ = {};

  std::tuple<std::string, std::string, vo::Egress> default_;
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_ROUTER_HPP
