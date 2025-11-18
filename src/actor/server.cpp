#include <algorithm>
#include <boost/asio/batch.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <format>
#include <iostream>
#include <memory>
#include <pichi/actor/detached.hpp>
#include <pichi/actor/server.hpp>
#include <pichi/common/error.hpp>
#include <pichi/vo/error.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <ranges>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <sstream>

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

static auto gen_resp(http::status status, std::string_view body)
{
  auto rep   = gen_resp(status);
  rep.body() = body;
  return rep;
}

static auto gen_resp(http::status status, json::Value const& body)
{
  auto buf    = json::StringBuffer{};
  auto writer = json::Writer<json::StringBuffer>{buf};
  body.Accept(writer);
  return gen_resp(status, buf.GetString());
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

static auto const INGRESS_REGEX      = std::regex{"^/ingresses/?([?#].*)?$"};
static auto const INGRESS_NAME_REGEX = std::regex{"^/ingresses/([^?#/]+)/?([?#].*)?$"};
static auto const EGRESS_REGEX       = std::regex{"^/egresses/?([?#].*)?$"};
static auto const EGRESS_NAME_REGEX  = std::regex{"^/egresses/([^?#]+)/?([?#].*)?$"};
static auto const RULE_REGEX         = std::regex{"^/rules/?([?#].*)?$"};
static auto const RULE_NAME_REGEX    = std::regex{"^/rules/([^?#]+)/?([?#].*)?$"};
static auto const ROUTE_REGEX        = std::regex{"^/route$"};

bool match(boost::string_view s, std::regex const& re, std::cmatch& mr)
{
  return std::regex_match(std::cbegin(s), std::cend(s), mr, re);
}

Server::Server(IOExecutor ex) : ex_{std::move(ex)}, router_{std::make_shared<Router>(ex_)}
{
  ingresses_.SetObject();
}

Awaitable<void> Server::serve(ip::tcp::endpoint endpoint)
{
  auto a = ip::tcp::acceptor{ex_, endpoint};
  while (a.is_open()) {
    asio::co_spawn(ex_, do_session(co_await a.async_accept(asio::use_awaitable)), detached);
  }
}

Awaitable<void> Server::do_session(ip::tcp::socket s)
{
  try {
    auto buf = beast::flat_buffer{};
    auto req = Request{};
    co_await http::async_read(s, buf, req, asio::use_awaitable);

    auto [ec, rep] = co_await redirect(handle(req));

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
  auto batch = asio::make_batch(ex_);
  co_await switch_to(batch);

  auto mr = std::cmatch{};
  if (match(req.target(), INGRESS_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::get:
      co_return gen_resp(http::status::ok, ingresses_);
    case http::verb::options:
      co_return gen_resp(http::verb::get, http::verb::options);
    default:
      break;
    }
  }
  else if (match(req.target(), INGRESS_NAME_REGEX, mr)) {
    auto name  = mr[1].str();
    auto alloc = json::Document::AllocatorType{};
    auto key   = vo::toJson(name, alloc);

    switch (req.method()) {
    case http::verb::delete_:
      listeners_.erase(name);
      ingresses_.EraseMember(key);
      co_return gen_resp(http::status::no_content);
    case http::verb::options:
      co_return gen_resp(http::verb::delete_, http::verb::options, http::verb::put);
    case http::verb::put: {
      auto vo   = vo::parse<vo::Ingress>(req.body());
      auto json = vo::toJson(vo, alloc);

      auto [it, _] = listeners_.insert_or_assign(name, Listener{ex_, name, router_, std::move(vo)});
      it->second.start();
      if (ingresses_.HasMember(key)) ingresses_.RemoveMember(key);
      ingresses_.AddMember(key, json, alloc);

      co_return gen_resp(http::status::no_content);
    }
    default:
      break;
    }
  }
  else if (match(req.target(), EGRESS_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::get:
      co_return gen_resp(http::status::ok, router_->get_egresses());
    case http::verb::options:
      co_return gen_resp(http::verb::get, http::verb::options);
    default:
      break;
    }
  }
  else if (match(req.target(), EGRESS_NAME_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::delete_:
      router_ = router_->del_egress(mr[1].str());
      co_return gen_resp(http::status::no_content);
    case http::verb::options:
      co_return gen_resp(http::verb::delete_, http::verb::options, http::verb::put);
    case http::verb::put:
      router_ = router_->put_egress(mr[1].str(), req.body());
      co_return gen_resp(http::status::no_content);
    default:
      break;
    }
  }
  else if (match(req.target(), RULE_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::get:
      co_return gen_resp(http::status::ok, router_->get_rules());
    case http::verb::options:
      co_return gen_resp(http::verb::get, http::verb::options);
    default:
      break;
    }
  }
  else if (match(req.target(), RULE_NAME_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::delete_:
      update_router(router_->del_rule(mr[1].str()));
      co_return gen_resp(http::status::no_content);
    case http::verb::options:
      co_return gen_resp(http::verb::delete_, http::verb::options, http::verb::put);
    case http::verb::put:
      update_router(router_->put_rule(mr[1].str(), req.body()));
      co_return gen_resp(http::status::no_content);
    default:
      break;
    }
  }
  else if (match(req.target(), ROUTE_REGEX, mr)) {
    switch (req.method()) {
    case http::verb::get:
      co_return gen_resp(http::status::ok, router_->get_route());
    case http::verb::options:
      co_return gen_resp(http::verb::get, http::verb::options, http::verb::put);
    case http::verb::put:
      update_router(router_->put_route(req.body()));
      co_return gen_resp(http::status::no_content);
    default:
      break;
    }
  }
  co_return gen_resp(http::status::not_found);
}

void Server::update_router(RouterPtr router)
{
  router_ = std::move(router);
  rngs::for_each(listeners_, [this](auto&& p) { p.second.reroute(router_); });
}

}  // namespace pichi::actor
