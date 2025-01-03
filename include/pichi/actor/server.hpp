#ifndef PICHI_ACTOR_SERVER_HPP
#define PICHI_ACTOR_SERVER_HPP

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <memory>
#include <pichi/actor/listener.hpp>
#include <pichi/actor/router.hpp>
#include <pichi/common/coro.hpp>

namespace pichi::actor {

class Server {
private:
  using RouterPtr = std::shared_ptr<Router>;

public:
  using HttpBody = boost::beast::http::string_body;
  using Request  = boost::beast::http::request<HttpBody>;
  using Response = boost::beast::http::response<HttpBody>;

  Server(IOExecutor, RouterPtr const&, Listener&);

  Awaitable<void> serve(boost::asio::ip::tcp::endpoint);

private:
  Awaitable<void>     do_session(boost::asio::ip::tcp::socket);
  Awaitable<Response> handle(Request const&);

  IOExecutor ex_;
  Listener&  listener_;
  RouterPtr  router_;
};

}  // namespace pichi::actor

#endif  // PICHI_ACTOR_SERVER_HPP
