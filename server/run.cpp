#include "pichi/common/config.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <pichi/actor/detached.hpp>
#include <pichi/actor/server.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/service/mmdb.hpp>
#include <pichi/vo/to_json.hpp>
#include <ranges>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <string_view>

#ifdef HAS_SIGNAL_H
#include <boost/asio/signal_set.hpp>
#include <signal.h>
#endif  // HAS_SIGNAL_H

using namespace pichi;
using namespace std::literals;
namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = boost::beast::http;
namespace json  = rapidjson;
namespace rngs  = std::ranges;
namespace views = rngs::views;

using ResolveResults = asio::ip::basic_resolver_results<asio::ip::tcp>;
using Socket         = asio::ip::tcp::socket;

static decltype(auto) INGRESSES = "ingresses";
static decltype(auto) EGRESSES  = "egresses";
static decltype(auto) RULES     = "rules";
static decltype(auto) ROUTE     = "route";
static decltype(auto) INDENT    = "  ";

static Awaitable<json::Document> get(Socket& s, std::string_view target)
{
  auto req = http::request<http::empty_body>{};
  req.method(http::verb::get);
  req.target(target);
  req.version(11);

  co_await http::async_write(s, req, asio::use_awaitable);

  auto buf  = beast::flat_static_buffer<1024>{};
  auto resp = http::response<http::string_body>{};

  co_await http::async_read(s, buf, resp, asio::use_awaitable);
  assertTrue(resp.result() == http::status::ok);

  auto doc = json::Document{};
  doc.Parse(resp.body().c_str());
  assertFalse(doc.HasParseError());
  co_return doc;
}

static Awaitable<http::status> put(Socket& s, std::string_view target, std::string_view body)
{
  auto req = http::request<http::string_body>{};
  req.method(http::verb::put);
  req.target(target);
  req.version(11);
  req.body() = body;
  req.prepare_payload();

  co_await http::async_write(s, req, asio::use_awaitable);

  auto buf  = beast::flat_static_buffer<1024>{};
  auto resp = http::response<http::string_body>{};

  co_await http::async_read(s, buf, resp, asio::use_awaitable);
  co_return resp.result();
}

static Awaitable<void> del(Socket& s, std::string_view target)
{
  auto req = http::request<http::string_body>{};
  req.method(http::verb::delete_);
  req.target(target);
  req.version(11);

  co_await http::async_write(s, req, asio::use_awaitable);

  auto buf  = beast::flat_static_buffer<1024>{};
  auto resp = http::response<http::string_body>{};

  co_await http::async_read(s, buf, resp, asio::use_awaitable);
  assertTrue(resp.result() == http::status::no_content);
}

class HttpClient {
private:
  template <typename StringRef>
  Awaitable<void> update(Socket& s, json::Value const& root, StringRef const& category)
  {
    auto it = root.FindMember(category);
    if (it == root.MemberEnd() || !it->value.IsObject()) {
      std::clog << std::format("{}{} NOT loaded\n", INDENT, category);
      co_return;
    }

    for (auto&& node : it->value.GetObj()) {
      auto target = std::format("/{}/{}", category, node.name.GetString());
      if (co_await put(s, target, vo::toString(node.value)) != http::status::no_content)
        std::clog << std::format("{}{} NOT loaded.\n", INDENT, target);
    }
  }

  Awaitable<void> reload(ResolveResults const& rr)
  {
    std::clog << std::format("Loading configuration {}\n", fn_);

    auto doc = json::Document{};
    auto ifs = std::ifstream{fn_};
    auto isw = json::IStreamWrapper{ifs};
    doc.ParseStream(isw);
    if (doc.HasParseError() || !doc.IsObject()) {
      std::clog << "Invalid JSON configuration\n";
      co_return;
    }

    auto sock = Socket{ex_};

    co_await asio::async_connect(sock, rr, asio::use_awaitable);
    co_await update(sock, doc, INGRESSES);
    co_await update(sock, doc, EGRESSES);
    co_await update(sock, doc, RULES);
    if (doc.HasMember(ROUTE) &&
        co_await put(sock, std::format("/{}", ROUTE), vo::toString(doc[ROUTE])) !=
            http::status::no_content)
      std::clog << std::format("{}{} NOT loaded\n", INDENT, ROUTE);

    std::clog << std::format("Configuration {} loaded\n", fn_);
  }

  Awaitable<void> flush(ResolveResults const& rr)
  {
    auto sock = Socket{ex_};

    co_await asio::async_connect(sock, rr, asio::use_awaitable);
    co_await put(sock, std::format("/{}/direct", EGRESSES), "{\"type\":\"direct\"}");
    co_await put(sock, std::format("/{}", ROUTE), "{\"default\":\"direct\",\"rules\":[]}");

    auto rules = co_await get(sock, std::format("/{}", RULES));
    for (auto&& r :
         rules.GetObj() | views::transform([](auto&& member) { return member.name.GetString(); })) {
      co_await del(sock, std::format("/{}/{}", RULES, r));
    }

    auto egresses = co_await get(sock, std::format("/{}", EGRESSES));
    for (auto&& e : egresses.GetObj() | views::transform([](auto&& member) {
                      return member.name.GetString();
                    }) | views::filter([](auto&& e) { return e != "direct"sv; })) {
      co_await del(sock, std::format("/{}/{}", EGRESSES, e));
    }

    auto ingresses = co_await get(sock, std::format("/{}", INGRESSES));
    for (auto&& i : ingresses.GetObj() |
                        views::transform([](auto&& member) { return member.name.GetString(); })) {
      co_await del(sock, std::format("/{}/{}", INGRESSES, i));
    }

    std::clog << "Configuration reset.\n";
  }

public:
  HttpClient(IOExecutor const& ex, std::string const& fn) : ex_{ex}, fn_{fn} {}

  Awaitable<void> run(std::string_view bind, uint16_t port)
  {
    if (rngs::empty(fn_)) co_return;

    auto rr = co_await asio::ip::tcp::resolver{ex_}
                  .async_resolve(bind, std::to_string(port), asio::use_awaitable);
    co_await reload(rr);

#if defined(HAS_SIGNAL_H) && defined(SIGHUP)
    auto ss = asio::signal_set{ex_};
    while (true) {
      ss.add(SIGHUP);

      co_await ss.async_wait(asio::use_awaitable);
      co_await flush(rr);
      co_await reload(rr);
    }
#endif  // defined(HAS_SIGNAL_H) && defined(SIGHUP)
  }

private:
  IOExecutor  ex_;
  std::string fn_;
};

void run(std::string const& bind, uint16_t port, std::string const& fn, std::string const& mmdb)
{
  auto io = asio::io_context{};
  auto ex = io.get_executor();

  auto server = actor::Server{ex};
  auto client = HttpClient{ex, fn};

  if (!mmdb.empty()) asio::use_service<service::Mmdb>(io).initialize(mmdb);

  asio::co_spawn(io, server.serve({asio::ip::make_address(bind), port}), actor::detached);
  asio::co_spawn(io, client.run(bind, port), [&](auto eptr) noexcept {
    if (eptr) {
      try {
        std::rethrow_exception(eptr);
      }
      catch (std::exception const& e) {
        std::clog << std::format("ERROR: {}\n", e.what());
      }
      io.stop();
    }
  });

  io.run();
}
