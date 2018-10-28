#include <algorithm>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/write.hpp>
#include <iostream>
#include <memory>
#include <pichi/api/rest.hpp>
#include <pichi/api/server.hpp>
#include <pichi/asserts.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>

using namespace std;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ip = asio::ip;
namespace json = rapidjson;
namespace sys = boost::system;
using tcp = ip::tcp;

namespace pichi::api {

static auto const INGRESS_REGEX = regex{"^/ingresses/?([?#].*)?$"};
static auto const INGRESS_NAME_REGEX = regex{"^/ingresses/([^?#/]+)/?([?#].*)?$"};
static auto const EGRESS_REGEX = regex{"^/egresses/?([?#].*)?$"};
static auto const EGRESS_NAME_REGEX = regex{"^/egresses/([^?#]+)/?([?#].*)?$"};
static auto const RULE_REGEX = regex{"^/rules/?([?#].*)?$"};
static auto const RULE_NAME_REGEX = regex{"^/rules/([^?#]+)/?([?#].*)?$"};
static auto const ROUTE_REGEX = regex{"^/route/?([?#].*)?$"};

static auto doc = json::Document{};
static auto& alloc = doc.GetAllocator();

static string serialize(json::Value const& v)
{
  auto buf = json::StringBuffer{};
  auto writer = json::Writer<json::StringBuffer>{buf};
  v.Accept(writer);
  return buf.GetString();
}

static auto defaultResponse(http::status status)
{
  auto ret = Server::Response{};
  ret.result(status);
  ret.version(11);
  if (status != http::status::no_content && status != http::status::not_found)
    ret.set(http::field::content_type, "application/json");
  return ret;
}

static bool matching(boost::string_view s, regex const& re, cmatch& r)
{
  return regex_match(s.data(), s.data() + s.size(), r, re);
}

static http::status mapCode(PichiError e)
{
  switch (e) {
  case PichiError::RES_IN_USE:
    return http::status::forbidden;
  case PichiError::BAD_JSON:
    return http::status::bad_request;
  case PichiError::SEMANTIC_ERROR:
    return http::status::unprocessable_entity;
  default:
    return http::status::internal_server_error;
  }
}

static void writeWithoutException(tcp::socket& s, Server::Response& resp, asio::yield_context yield)
{
  auto ec = sys::error_code{};
  http::async_write(s, resp, yield[ec]);
  if (ec) cout << "Ignoring HTTP error: " << ec.message() << endl;
}

static void replyError(tcp::socket& s, asio::yield_context yield, ErrorVO const& error,
                       http::status code = http::status::internal_server_error)
{
  auto resp = defaultResponse(code);
  resp.body() = serialize(toJson(error, alloc));
  writeWithoutException(s, resp, yield);
}

template <typename InputIt>
void dispatch(InputIt first, InputIt last, tcp::socket& s, asio::yield_context yield)
{
  auto buf = beast::flat_static_buffer<net::MAX_FRAME_SIZE>{};
  auto req = Server::Request{};

  http::async_read(s, buf, req, yield);
  auto mr = cmatch{};
  auto it = find_if(first, last, [v = req.method(), t = req.target(), &mr](auto&& item) {
    return get<0>(item) == v && matching(t, get<1>(item), mr);
  });
  auto resp = it != last ? invoke(get<2>(*it), req, mr) : defaultResponse(http::status::not_found);
  writeWithoutException(s, resp, yield);
}

template <typename Manager> auto getVO(Manager const& manager)
{
  auto resp = defaultResponse(http::status::ok);
  resp.body() = serialize(toJson(begin(manager), end(manager), alloc));
  return resp;
}

template <typename Manager>
auto putVO(typename Manager::VO&& vo, cmatch const& mr, Manager& manager)
{
  manager.update(mr[1].str(), move(vo));
  return defaultResponse(http::status::no_content);
}

template <typename Manager>
auto putVO(Server::Request const& req, cmatch const& mr, Manager& manager)
{
  return putVO(parse<typename Manager::VO>(req.body()), mr, manager);
}

template <typename Manager> auto delVO(cmatch const& mr, Manager& manager)
{
  manager.erase(mr[1].str());
  return defaultResponse(http::status::no_content);
}

static auto options(initializer_list<http::verb>&& verbs)
{
  auto oss = ostringstream{};
  auto first = cbegin(verbs);
  if (first != cend(verbs)) oss << *first++;
  for_each(first, cend(verbs), [&oss](auto verb) { oss << "," << verb; });

  auto resp = defaultResponse(http::status::no_content);
  resp.set("Access-Control-Allow-Methods", oss.str());
  return resp;
}

Server::Server(asio::io_context& io, char const* fn)
  : strand_{io}, router_{fn}, eManager_{}, iManager_{strand_, router_, eManager_},
    apis_{
        make_tuple(http::verb::get, INGRESS_REGEX,
                   [this](auto&&, auto&&) { return getVO(iManager_); }),
        make_tuple(http::verb::options, INGRESS_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::get, http::verb::options});
                   }),
        make_tuple(http::verb::put, INGRESS_NAME_REGEX,
                   [this](auto&& r, auto&& mr) { return putVO(r, mr, iManager_); }),
        make_tuple(http::verb::delete_, INGRESS_NAME_REGEX,
                   [this](auto&&, auto&& mr) { return delVO(mr, iManager_); }),
        make_tuple(http::verb::options, INGRESS_NAME_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::put, http::verb::delete_, http::verb::options});
                   }),
        make_tuple(http::verb::get, EGRESS_REGEX,
                   [this](auto&&, auto&&) { return getVO(eManager_); }),
        make_tuple(http::verb::options, EGRESS_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::get, http::verb::options});
                   }),
        make_tuple(http::verb::put, EGRESS_NAME_REGEX,
                   [this](auto&& r, auto&& mr) { return putVO(r, mr, eManager_); }),
        make_tuple(http::verb::delete_, EGRESS_NAME_REGEX,
                   [this](auto&&, auto&& mr) {
                     assertFalse(router_.isUsed(mr[1].str()), PichiError::RES_IN_USE);
                     return delVO(mr, eManager_);
                   }),
        make_tuple(http::verb::options, EGRESS_NAME_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::put, http::verb::delete_, http::verb::options});
                   }),
        make_tuple(http::verb::get, RULE_REGEX, [this](auto&&, auto&&) { return getVO(router_); }),
        make_tuple(http::verb::options, RULE_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::get, http::verb::options});
                   }),
        make_tuple(
            http::verb::put, RULE_NAME_REGEX,
            [this](auto&& r, auto&& mr) { return putVO(parse<RuleVO>(r.body()), mr, router_); }),
        make_tuple(http::verb::delete_, RULE_NAME_REGEX,
                   [this](auto&&, auto&& mr) { return delVO(mr, router_); }),
        make_tuple(http::verb::options, RULE_NAME_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::put, http::verb::delete_, http::verb::options});
                   }),
        make_tuple(http::verb::get, ROUTE_REGEX,
                   [this](auto&&, auto&&) {
                     auto resp = defaultResponse(http::status::ok);
                     resp.body() = serialize(toJson(router_.getRoute(), alloc));
                     return resp;
                   }),
        make_tuple(http::verb::put, ROUTE_REGEX,
                   [this](auto&& r, auto&& mr) {
                     auto vo = parse<RouteVO>(r.body());
                     assertFalse(vo.default_.has_value() &&
                                     eManager_.find(*vo.default_) == end(eManager_),
                                 PichiError::RES_IN_USE);
                     router_.setRoute(move(vo));
                     return defaultResponse(http::status::no_content);
                   }),
        make_tuple(http::verb::options, ROUTE_REGEX, [](auto&&, auto&&) {
          return options({http::verb::get, http::verb::put, http::verb::options});
        })}
{
}

void Server::listen(string_view address, uint16_t port)
{
  asio::spawn(strand_, [a = tcp::acceptor{strand_.context(), {ip::make_address(address), port}},
                        this](auto yield) mutable {
    auto ec = sys::error_code{};
    while (!ec) {
      asio::spawn(strand_, [first = cbegin(apis_), last = cend(apis_),
                            s = a.async_accept(yield)](auto yield) mutable {
        try {
          dispatch(first, last, s, yield);
        }
        catch (Exception const& e) {
          cout << "Pichi Error: " << e.what() << endl;
          replyError(s, yield, {e.what()}, mapCode(e.error()));
        }
        catch (sys::system_error const& e) {
          if (e.code() == asio::error::eof || e.code() == asio::error::operation_aborted) return;
          cout << "Socket Error: " << e.what() << endl;
          replyError(s, yield, {e.what()},
                     e.code() == asio::error::address_in_use ? http::status::locked :
                                                               http::status::internal_server_error);
        }
      });
    }
    cout << "Socket Error: " << ec.message() << endl;
  });
}

} // namespace pichi::api
