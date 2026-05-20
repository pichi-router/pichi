#ifndef PICHI_ACTOR_SERVER_HPP
#define PICHI_ACTOR_SERVER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <memory>
#include <pichi/actor/listener.hpp>
#include <pichi/actor/router.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/rule.hpp>
#include <unordered_map>

namespace pichi::actor {

class Server {
private:
  template <typename Value> using ValueMap = std::unordered_map<std::string, Value>;

  using Strand    = boost::asio::strand<IOExecutor>;
  using RouterPtr = std::shared_ptr<Router>;

public:
  using HttpBody = boost::beast::http::string_body;
  using Request  = boost::beast::http::request<HttpBody>;
  using Response = boost::beast::http::response<HttpBody>;

  explicit Server(IOExecutor const&);

  Awaitable<void> serve(boost::asio::ip::tcp::endpoint);

private:
  Awaitable<void>     do_session(boost::asio::ip::tcp::socket);
  Awaitable<Response> handle(Request const&);

  void update_router();

  Strand strand_;

  ValueMap<Listener>   listeners_ = {};
  ValueMap<vo::Egress> egresses_;
  ValueMap<vo::Rule>   rules_ = {};

  vo::Route route_;
  RouterPtr router_;
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_SERVER_HPP
