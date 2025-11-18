#ifndef PICHI_ACTOR_LISTENER_HPP
#define PICHI_ACTOR_LISTENER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <pichi/actor/router.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/vo/ingress.hpp>
#include <string>
#include <vector>

namespace pichi::actor {

class Listener {
private:
  using Acceptor  = boost::asio::ip::tcp::acceptor;
  using Acceptors = std::vector<Acceptor>;
  using RouterPtr = std::shared_ptr<Router>;
  using Strand    = boost::asio::strand<IOExecutor>;

  Awaitable<void> listen(Acceptor&);

public:
  Listener(IOExecutor const&, std::string const&, RouterPtr const&, vo::Ingress);

  void start();

  void reroute(RouterPtr const&);

private:
  Strand      strand_;
  RouterPtr   router_;
  std::string name_;
  vo::Ingress vo_;
  Acceptors   acceptors_ = {};
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_LISTENER_HPP
