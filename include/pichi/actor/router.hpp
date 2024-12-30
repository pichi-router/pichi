#ifndef PICHI_ACTOR_ROUTER_HPP
#define PICHI_ACTOR_ROUTER_HPP

#include <pichi/common/coro.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/rule.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pichi::actor {

class Router {
private:
  using Pointer = std::shared_ptr<Router>;

  using Egresses = std::unordered_map<std::string, vo::Egress>;
  using Rules    = std::unordered_map<std::string, vo::Rule>;

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

  Awaitable<vo::Egress> route(Endpoint const&) const;

private:
  IOExecutor ex_;
  Egresses   egresses_;
  Rules      rules_ = {};
  vo::Route  route_;
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_ROUTER_HPP
