#ifndef PICHI_ACTOR_SESSION_HPP
#define PICHI_ACTOR_SESSION_HPP

#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <pichi/actor/router.hpp>
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/vo/ingress.hpp>

namespace pichi::actor {

class Session {
private:
  using RouterPtr = std::shared_ptr<Router>;
  using Socket    = boost::asio::ip::tcp::socket;

  Awaitable<adapter::tcp::Egress> handshake(adapter::tcp::Ingress&, AdapterType);

public:
  template <boost::asio::execution::executor Executor>
  Session(Executor ex, RouterPtr r, std::string_view iname)
    : ex_{std::move(ex)}, router_{std::move(r)}, iname_{iname}
  {
  }

  Awaitable<void> start(vo::Ingress const&, Socket);

private:
  IOExecutor       ex_;
  RouterPtr        router_;
  std::string_view iname_;
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_SESSION_HPP
