#ifndef PICHI_ACTOR_ROUTER_HPP
#define PICHI_ACTOR_ROUTER_HPP

#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include <map>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
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
  template <typename Results> bool match_range(Endpoint const&, Results&&) const;

  bool match_pattern(std::string const&) const;

  bool match_domain(Endpoint const&) const;

public:
  Matcher(std::string_view, vo::Rule const&);

  template <typename Results>
  bool match(Endpoint const&, std::string_view, AdapterType, Results&&) const;

  bool need_resolving() const;

  std::string_view name() const;

private:
  std::string_view name_;

  std::vector<boost::asio::ip::network_v4> ranges4_ = {};
  std::vector<boost::asio::ip::network_v6> ranges6_ = {};

  std::unordered_set<std::string_view> inames_ = {};

  std::unordered_set<AdapterType> types_ = {};

  std::vector<std::regex> patterns_ = {};

  std::vector<std::string>      domains_   = {};
  std::vector<std::string_view> countries_ = {};
};

}  // namespace detail

class Router {
private:
  using Pointer = std::shared_ptr<Router>;

  using Egresses = std::unordered_map<std::string, vo::Egress>;
  using Rules    = std::unordered_map<std::string, vo::Rule>;

  using Matchers = std::map<std::string_view, std::vector<detail::Matcher>>;

  void build_matchers();

public:
  Router(IOExecutor);

  std::string get_egresses() const;
  Pointer     del_egress(std::string const&);
  Pointer     put_egress(std::string const&, std::string_view);

  std::string get_rules() const;
  Pointer     del_rule(std::string const&);
  Pointer     put_rule(std::string const&, std::string_view);

  std::string get_route() const;
  Pointer     put_route(std::string_view);

  Awaitable<std::tuple<std::string, std::string, vo::Egress>>
      route(Endpoint const&, std::string_view, AdapterType) const;

private:
  IOExecutor ex_;

  Egresses  egresses_;
  Rules     rules_ = {};
  vo::Route route_;

  Matchers matchers_ = {};
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_ROUTER_HPP
