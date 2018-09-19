#ifndef PICHI_API_SERVER_HPP
#define PICHI_API_SERVER_HPP

#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <functional>
#include <pichi/api/egress_manager.hpp>
#include <pichi/api/ingress_manager.hpp>
#include <pichi/api/router.hpp>
#include <regex>
#include <string_view>
#include <tuple>

namespace pichi::api {

class Server {
public:
  using HttpBody = boost::beast::http::string_body;
  using Request = boost::beast::http::request<HttpBody>;
  using Response = boost::beast::http::response<HttpBody>;
  using HttpHandler = std::function<Response(Request const&, std::cmatch const&)>;
  using RouteItem = std::tuple<boost::beast::http::verb, std::regex, HttpHandler>;

public:
  Server(Server const&) = delete;
  Server(Server&&) = delete;
  Server& operator=(Server const&) = delete;
  Server& operator=(Server&&) = delete;

  Server(boost::asio::io_context&, char const*);
  ~Server() = default;

  void listen(std::string_view, uint16_t);

private:
  boost::asio::io_context::strand strand_;
  Router router_;
  EgressManager eManager_;
  IngressManager iManager_;
  std::array<RouteItem, 18> apis_;
};

} // namespace pichi::api

#endif // PICHI_API_SERVER_HPP
