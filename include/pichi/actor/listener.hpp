#ifndef PICHI_ACTOR_LISTENER_HPP
#define PICHI_ACTOR_LISTENER_HPP

#include <boost/asio/strand.hpp>
#include <memory>
#include <pichi/actor/router.hpp>
#include <pichi/api/ingress_holder.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/vo/ingress.hpp>
#include <string>
#include <unordered_map>

namespace pichi::actor {

class Listener {
private:
  using Ingresses = std::unordered_map<std::string, api::IngressHolder>;
  using RouterPtr = std::shared_ptr<Router>;
  using Strand    = boost::asio::strand<IOExecutor>;

  Awaitable<void> listen(std::string_view, boost::asio::ip::tcp::acceptor&, api::IngressHolder&);

public:
  Listener(IOExecutor, RouterPtr const&);

  Awaitable<std::string> get_ingresses() const;
  Awaitable<void>        del_ingress(std::string const&);
  Awaitable<void>        put_ingress(std::string const&, std::string_view);

  Awaitable<void> set_router(RouterPtr const&);

private:
  Strand    strand_;
  RouterPtr router_;
  Ingresses ingresses_;
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_LISTENER_HPP
