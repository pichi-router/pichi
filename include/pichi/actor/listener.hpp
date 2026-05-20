#ifndef PICHI_ACTOR_LISTENER_HPP
#define PICHI_ACTOR_LISTENER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <pichi/actor/router.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/vo/ingress.hpp>
#include <vector>

namespace pichi::actor {

class Listener {
private:
  using Acceptor  = boost::asio::ip::tcp::acceptor;
  using Acceptors = std::vector<Acceptor>;
  using RouterPtr = std::shared_ptr<Router>;
  using Strand    = boost::asio::strand<IOExecutor>;
  using Ingress   = vo::Ingress;

  Awaitable<void> listen(Acceptor&);

public:
  Listener(IOExecutor const&, RouterPtr const&, Ingress);

  void start();

  void reroute(RouterPtr const&);

  Ingress const& vo() const;

private:
  Strand    strand_;
  RouterPtr router_;
  Ingress   vo_;
  Acceptors acceptors_ = {};
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_LISTENER_HPP
