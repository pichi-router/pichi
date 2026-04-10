#include <algorithm>
#include <boost/asio/redirect_error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <pichi/actor/detached.hpp>
#include <pichi/actor/server.hpp>
#include <pichi/common/error.hpp>
#include <pichi/vo/error.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <ranges>
#include <rapidjson/document.h>
#include <regex>
#include <sstream>

using namespace std::literals;

namespace asio  = boost::asio;
namespace ip    = asio::ip;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace json  = rapidjson;
namespace rngs  = std::ranges;
namespace sys   = boost::system;

namespace pichi::actor {

template <typename T, typename... U>
concept AllOf = (std::same_as<T, U> && ...);

static auto gen_resp(http::status status) { return Server::Response{status, 11}; }

static auto gen_resp(http::status status, json::Value const& body)
{
  auto rep   = gen_resp(status);
  rep.body() = vo::toString(body);
  return rep;
}

static auto gen_resp(sys::error_code const& ec)
{
  auto status = http::status::internal_server_error;
  if (ec == asio::error::address_in_use)
    status = http::status::locked;
  else if (ec == PichiError::RES_IN_USE)
    status = http::status::forbidden;
  else if (ec == PichiError::BAD_JSON)
    status = http::status::bad_request;
  else if (ec == PichiError::SEMANTIC_ERROR)
    status = http::status::unprocessable_entity;

  auto alloc = json::Document::AllocatorType{};
  return gen_resp(status, vo::toJson(vo::Error{ec.message()}, alloc));
}

template <typename... Verbs>
requires((sizeof...(Verbs) > 1) && ... && std::same_as<http::verb, Verbs>)
auto gen_resp(Verbs... verbs)
{
  auto resp = gen_resp(http::status::no_content);
  resp.set(
      "Access-Control-Allow-Methods",
      std::apply(
          [](auto first, auto... rests) {
            auto oss    = std::ostringstream{};
            auto append = [&](auto v) { oss << "," << v; };
            ((oss << first), ..., append(rests));
            return oss.str();
          },
          std::make_tuple(verbs...)
      )
  );
  return resp;
}

template <typename Value, typename Alloc>
auto to_json(std::unordered_map<std::string, Value> const& m, Alloc& alloc)
{
  auto ret = json::Value{};

  ret.SetObject();
  rngs::for_each(m, [&](auto&& item) {
    auto&& [name, value] = item;
    assertFalse(name.empty());
    ret.AddMember(vo::toJson(name, alloc), vo::toJson(value, alloc), alloc);
  });
  return ret;
}

static auto const INGRESS_REGEX      = std::regex{"^/ingresses/?([?#].*)?$"};
static auto const INGRESS_NAME_REGEX = std::regex{"^/ingresses/([^?#/]+)/?([?#].*)?$"};
static auto const EGRESS_REGEX       = std::regex{"^/egresses/?([?#].*)?$"};
static auto const EGRESS_NAME_REGEX  = std::regex{"^/egresses/([^?#]+)/?([?#].*)?$"};
static auto const RULE_REGEX         = std::regex{"^/rules/?([?#].*)?$"};
static auto const RULE_NAME_REGEX    = std::regex{"^/rules/([^?#]+)/?([?#].*)?$"};
static auto const ROUTE_REGEX        = std::regex{"^/route$"};

static auto const DEFAULT_EGRESS_NAME = "direct"s;

bool match(boost::string_view s, std::regex const& re, std::cmatch& mr)
{
  return std::regex_match(std::cbegin(s), std::cend(s), mr, re);
}

Server::Server(IOExecutor const& ex)
: strand_{asio::make_strand(ex)},
  egresses_{
    {DEFAULT_EGRESS_NAME, vo::Egress{.type_ = AdapterType::DIRECT}}
  },
  route_{vo::Route{.default_ = DEFAULT_EGRESS_NAME}},
  router_{std::make_shared<Router>(ex, egresses_, rules_, route_)}
{
}

Awaitable<void> Server::serve(ip::tcp::endpoint endpoint)
{
  auto ex = strand_.get_inner_executor();
  auto ac = ip::tcp::acceptor{ex, endpoint};
  while (ac.is_open())
    asio::co_spawn(ex, do_session(co_await ac.async_accept(asio::use_awaitable)), detached);
}

Awaitable<void> Server::do_session(ip::tcp::socket s)
{
  try {
    auto buf    = beast::flat_buffer{};
    auto parser = http::request_parser<HttpBody>{};
    parser.body_limit(std::numeric_limits<uint64_t>::max());
    co_await http::async_read(s, buf, parser, asio::use_awaitable);

    auto [ec, rep] = co_await redirect(handle(parser.release()));

    if (ec) rep.emplace(gen_resp(ec));
    co_await http::async_write(s, *rep, asio::use_awaitable);
  }
  catch (sys::system_error const& e) {
    std::cout << std::format("API IO Error: {}\n", e.what());
  }
  catch (std::exception const& e) {
    std::clog << std::format("ERROR: {}\n", e.what());
  }
}

Awaitable<Server::Response> Server::handle(Request const& req)
{
  auto ex    = strand_.get_inner_executor();
  auto mr    = std::cmatch{};
  auto alloc = json::Document::AllocatorType{};

  co_await switch_to(strand_);

  if (match(req.target(), INGRESS_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::get:
      co_return gen_resp(http::status::ok, to_json(ingresses_, alloc));
    case http::verb::options:
      co_return gen_resp(http::verb::get, http::verb::options);
    default:
      break;
    }
  }
  else if (match(req.target(), INGRESS_NAME_REGEX, mr)) {
    auto name = mr[1].str();
    auto key  = vo::toJson(name, alloc);

    switch (req.method()) {
    case http::verb::delete_:
      listeners_.erase(name);
      ingresses_.erase(name);
      co_return gen_resp(http::status::no_content);
    case http::verb::options:
      co_return gen_resp(http::verb::delete_, http::verb::options, http::verb::put);
    case http::verb::put: {
      auto vo      = vo::parse<vo::Ingress>(req.body());
      auto [it, _] = listeners_.insert_or_assign(name, Listener{ex, name, router_, vo});
      it->second.start();
      ingresses_.insert_or_assign(name, std::move(vo));

      co_return gen_resp(http::status::no_content);
    }
    default:
      break;
    }
  }
  else if (match(req.target(), EGRESS_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::get:
      co_return gen_resp(http::status::ok, to_json(egresses_, alloc));
    case http::verb::options:
      co_return gen_resp(http::verb::get, http::verb::options);
    default:
      break;
    }
  }
  else if (match(req.target(), EGRESS_NAME_REGEX, mr)) {
    auto ename  = mr[1].str();
    auto in_use = [&]() {
      return *route_.default_ == ename ||
             rngs::any_of(route_.rules_, [&](auto&& item) { return item.second == ename; });
    };
    switch (req.method()) {
    case http::verb::delete_:
      assertFalse(in_use(), PichiError::RES_IN_USE);
      egresses_.erase(ename);
      co_return gen_resp(http::status::no_content);
    case http::verb::options:
      co_return gen_resp(http::verb::delete_, http::verb::options, http::verb::put);
    case http::verb::put:
      egresses_.insert_or_assign(ename, vo::parse<vo::Egress>(req.body()));
      if (in_use()) update_router();
      co_return gen_resp(http::status::no_content);
    default:
      break;
    }
  }
  else if (match(req.target(), RULE_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::get:
      co_return gen_resp(http::status::ok, to_json(rules_, alloc));
    case http::verb::options:
      co_return gen_resp(http::verb::get, http::verb::options);
    default:
      break;
    }
  }
  else if (match(req.target(), RULE_NAME_REGEX, mr)) {
    auto rname  = mr[1].str();
    auto in_use = [&]() {
      return rngs::any_of(route_.rules_, [&](auto&& item) {
        return rngs::any_of(item.first, [&](auto&& rn) { return rn == rname; });
      });
    };
    switch (req.method()) {
    case http::verb::delete_:
      assertFalse(in_use(), PichiError::RES_IN_USE);
      rules_.erase(rname);
      co_return gen_resp(http::status::no_content);
    case http::verb::options:
      co_return gen_resp(http::verb::delete_, http::verb::options, http::verb::put);
    case http::verb::put:
      rules_.insert_or_assign(rname, vo::parse<vo::Rule>(req.body()));
      if (in_use()) update_router();
      co_return gen_resp(http::status::no_content);
    default:
      break;
    }
  }
  else if (match(req.target(), ROUTE_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::get:
      co_return gen_resp(http::status::ok, vo::toJson(route_, alloc));
    case http::verb::options:
      co_return gen_resp(http::verb::get, http::verb::options, http::verb::put);
    case http::verb::put: {
      auto route = vo::parse<vo::Route>(req.body());
      assertTrue(
          rngs::all_of(
              route.rules_,
              [this](auto&& item) {
                return rngs::all_of(item.first, [this](auto&& rname) {
                  return rules_.contains(rname);
                });
              }
          ),
          PichiError::SEMANTIC_ERROR
      );
      assertTrue(
          rngs::all_of(
              route.rules_,
              [this](auto&& item) { return egresses_.contains(item.second); }
          ),
          PichiError::SEMANTIC_ERROR
      );
      if (route.default_)
        assertTrue(egresses_.contains(*route.default_), PichiError::SEMANTIC_ERROR);
      route_ = route;
      update_router();
      co_return gen_resp(http::status::no_content);
    }
    default:
      break;
    }
  }
  co_return gen_resp(http::status::not_found);
}

void Server::update_router()
{
  router_ = std::make_shared<Router>(strand_.get_inner_executor(), egresses_, rules_, route_);
  rngs::for_each(listeners_, [this](auto&& p) { p.second.reroute(router_); });
}

}  // namespace pichi::actor
