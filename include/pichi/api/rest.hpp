#ifndef PICHI_API_REST_HPP
#define PICHI_API_REST_HPP

#include <array>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <exception>
#include <functional>
#include <regex>
#include <tuple>

namespace pichi::api {

class EgressManager;
class IngressManager;
class Router;

class Rest {
public:
  using HttpBody = boost::beast::http::string_body;
  using Request = boost::beast::http::request<HttpBody>;
  using Response = boost::beast::http::response<HttpBody>;

  explicit Rest(IngressManager&, EgressManager&, Router&);
  Response handle(Request const&);

  static Response errorResponse(std::exception_ptr);

private:
  using HttpHandler = std::function<Response(Request const&, std::cmatch const&)>;
  using RouteItem = std::tuple<boost::beast::http::verb, std::regex, HttpHandler>;

  std::array<RouteItem, 18> apis_;
};

} // namespace pichi::api

#endif // PICHI_API_REST_HPP
