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

static auto const INBOUND_REGEX = regex{"^/inbound/?([?#].*)?$"};
static auto const INBOUND_NAME_REGEX = regex{"^/inbound/([^?#/]+)/?([?#].*)?$"};
static auto const OUTBOUND_REGEX = regex{"^/outbound/?([?#].*)?$"};
static auto const OUTBOUND_NAME_REGEX = regex{"^/outbound/([^?#]+)/?([?#].*)?$"};
static auto const RULE_REGEX = regex{"^/rules/?([?#].*)?$"};
static auto const RULE_NAME_REGEX = regex{"^/rules/([^?#]+)/?([?#].*)?$"};
static auto const ROUTE_REGEX = regex{"^/route/?([?#].*)?$"};

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
  if (status != http::status::no_content) ret.set(http::field::content_type, "application/json");
  return ret;
}

static bool matching(boost::string_view s, regex const& re, cmatch& r)
{
  return regex_match(s.data(), s.data() + s.size(), r, re);
}

static void writeWithoutException(tcp::socket& s, Server::Response& resp, asio::yield_context ctx)
{
  auto ec = sys::error_code{};
  http::async_write(s, resp, ctx[ec]);
  if (ec) cout << "Ignoring HTTP error: " << ec.message() << "\n";
}

static void replyError(exception_ptr eptr, tcp::socket& s, asio::yield_context ctx)
{
  auto resp = defaultResponse(http::status::internal_server_error);
  try {
    if (eptr) rethrow_exception(eptr);
  }
  catch (exception& e) {
    resp.body() = e.what();
  }
  writeWithoutException(s, resp, ctx);
}

static void logging(exception_ptr eptr)
{
  try {
    if (eptr) rethrow_exception(eptr);
  }
  catch (Exception& e) {
    cout << "Pichi Error: " << e.what() << "\n";
  }
  catch (sys::system_error& e) {
    if (e.code() == asio::error::eof || e.code() == asio::error::operation_aborted) return;
    cout << "Socket Error: " << e.what() << "\n";
  }
}

template <typename Manager> auto getVO(Manager const& manager)
{
  auto doc = json::Document{};
  auto resp = defaultResponse(http::status::ok);
  resp.body() = serialize(toJson(begin(manager), end(manager), doc.GetAllocator()));
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

template <typename Function> void Server::spawn(Function&& func)
{
  spawn(forward<Function>(func), [](auto, auto) {});
}

template <typename Function, typename FaultHandler>
void Server::spawn(Function&& func, FaultHandler&& handler)
{
  asio::spawn(strand_, [f = forward<Function>(func), h = forward<FaultHandler>(handler),
                        this](auto ctx) mutable {
    try {
      f(ctx);
    }
    catch (...) {
      auto eptr = current_exception();
      h(eptr, ctx);
      logging(eptr);
    }
  });
}

template <typename Socket, typename Yield> void Server::route(Socket& s, Yield yield)
{
  auto buf = beast::flat_static_buffer<net::MAX_FRAME_SIZE>{};
  auto req = Request{};

  http::async_read(s, buf, req, yield);
  auto mr = cmatch{};
  auto it = find_if(cbegin(routes_), cend(routes_),
                    [v = req.method(), t = req.target(), &mr](auto&& item) {
                      return get<0>(item) == v && matching(t, get<1>(item), mr);
                    });
  assertFalse(it == cend(routes_), PichiError::MISC);
  auto resp = invoke(get<2>(*it), req, mr);
  writeWithoutException(s, resp, yield);
}

Server::Server(asio::io_context& io, char const* fn)
  : strand_{io}, router_{fn}, egress_{}, ingress_{strand_, router_, egress_},
    routes_{
        make_tuple(http::verb::get, INBOUND_REGEX,
                   [this](auto&&, auto&&) { return getVO(ingress_); }),
        make_tuple(http::verb::options, INBOUND_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::get, http::verb::options});
                   }),
        make_tuple(http::verb::put, INBOUND_NAME_REGEX,
                   [this](auto&& r, auto&& mr) { return putVO(r, mr, ingress_); }),
        make_tuple(http::verb::delete_, INBOUND_NAME_REGEX,
                   [this](auto&&, auto&& mr) { return delVO(mr, ingress_); }),
        make_tuple(http::verb::options, INBOUND_NAME_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::put, http::verb::delete_, http::verb::options});
                   }),
        make_tuple(http::verb::get, OUTBOUND_REGEX,
                   [this](auto&&, auto&&) { return getVO(egress_); }),
        make_tuple(http::verb::options, OUTBOUND_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::get, http::verb::options});
                   }),
        make_tuple(http::verb::put, OUTBOUND_NAME_REGEX,
                   [this](auto&& r, auto&& mr) { return putVO(r, mr, egress_); }),
        make_tuple(http::verb::delete_, OUTBOUND_NAME_REGEX,
                   [this](auto&&, auto&& mr) {
                     // TODO use the correct exception
                     assertFalse(router_.isUsed(mr[1].str()), PichiError::MISC);
                     return delVO(mr, egress_);
                   }),
        make_tuple(http::verb::options, OUTBOUND_NAME_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::put, http::verb::delete_, http::verb::options});
                   }),
        make_tuple(http::verb::get, RULE_REGEX, [this](auto&&, auto&&) { return getVO(router_); }),
        make_tuple(http::verb::options, RULE_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::get, http::verb::options});
                   }),
        make_tuple(http::verb::put, RULE_NAME_REGEX,
                   [this](auto&& r, auto&& mr) {
                     // TODO use the correct exception
                     auto vo = parse<RuleVO>(r.body());
                     assertFalse(egress_.find(vo.outbound_) == end(egress_), PichiError::MISC);
                     return putVO(move(vo), mr, router_);
                   }),
        make_tuple(http::verb::delete_, RULE_NAME_REGEX,
                   [this](auto&&, auto&& mr) { return delVO(mr, router_); }),
        make_tuple(http::verb::options, RULE_NAME_REGEX,
                   [](auto&&, auto&&) {
                     return options({http::verb::put, http::verb::delete_, http::verb::options});
                   }),
        make_tuple(http::verb::get, ROUTE_REGEX,
                   [this](auto&&, auto&&) {
                     auto doc = json::Document{};
                     auto resp = defaultResponse(http::status::ok);
                     resp.body() = serialize(toJson(router_.getRoute(), doc.GetAllocator()));
                     return resp;
                   }),
        make_tuple(http::verb::put, ROUTE_REGEX,
                   [this](auto&& r, auto&& mr) {
                     auto vo = parse<RouteVO>(r.body());
                     assertFalse(vo.default_.has_value() &&
                                     egress_.find(vo.default_.value()) == end(egress_),
                                 // TODO use the correct exception
                                 PichiError::MISC);
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
  spawn([port, address, this](auto ctx) {
    auto acceptor = tcp::acceptor{strand_.context(), {ip::make_address(address), port}};
    while (true) {
      auto sp = make_shared<tcp::socket>(acceptor.async_accept(ctx));
      spawn([sp, this](auto ctx) mutable { route(*sp, ctx); },
            [sp](auto eptr, auto ctx) mutable { replyError(eptr, *sp, ctx); });
    }
  });
}

} // namespace pichi::api
