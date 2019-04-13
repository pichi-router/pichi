#include "config.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <iostream>
#include <pichi/api/server.hpp>
#include <pichi/asserts.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/spawn.hpp>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

#ifdef HAS_SIGNAL_H
#include <boost/asio/signal_set.hpp>
#include <signal.h>
#endif // HAS_SIGNAL_H

using namespace std;
using namespace pichi;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace fs = boost::filesystem;
namespace http = boost::beast::http;
namespace ip = asio::ip;
namespace json = rapidjson;

using ip::tcp;

static decltype(auto) INGRESSES = "ingresses";
static decltype(auto) EGRESSES = "egresses";
static decltype(auto) RULES = "rules";
static decltype(auto) ROUTE = "route";
static decltype(auto) INDENT = "  ";

static asio::io_context io{1};
static auto alloc = json::Document::AllocatorType{};

static json::Document parseJson(char const* str)
{
  auto doc = json::Document{};
  doc.Parse(str);
  assertFalse(doc.HasParseError());
  return doc;
}

class HttpHelper {
public:
  HttpHelper(string const& host, uint16_t port, asio::yield_context yield);

  vector<string> get(string const& target);
  void put(string const& target, string const& body);
  void del(string const& target);

private:
  net::Endpoint endpoint_;
  asio::yield_context yield_;
};

HttpHelper::HttpHelper(string const& host, uint16_t port, asio::yield_context yield)
  : endpoint_{net::makeEndpoint(host, port)}, yield_{yield}
{
}

vector<string> HttpHelper::get(string const& target)
{
  auto s = tcp::socket{io};
  net::connect(endpoint_, s, yield_);

  auto req = http::request<http::empty_body>{};
  req.method(http::verb::get);
  req.target(target);
  req.version(11);
  http::async_write(s, req, yield_);

  auto buf = beast::flat_static_buffer<1024>{};
  auto resp = http::response<http::string_body>{};
  http::async_read(s, buf, resp, yield_);
  assertTrue(resp.result() == http::status::ok);

  auto doc = parseJson(resp.body().c_str());
  auto ret = vector<string>{};
  transform(doc.MemberBegin(), doc.MemberEnd(), back_inserter(ret),
            [](auto&& member) { return member.name.GetString(); });
  return ret;
}

void HttpHelper::put(string const& target, string const& body)
{
  auto s = tcp::socket{io};
  net::connect(endpoint_, s, yield_);

  auto req = http::request<http::string_body>{};
  req.method(http::verb::put);
  req.target(target);
  req.version(11);
  req.body() = body;
  req.prepare_payload();
  http::async_write(s, req, yield_);

  auto buf = beast::flat_static_buffer<1024>{};
  auto resp = http::response<http::string_body>{};
  http::async_read(s, buf, resp, yield_);
  if (resp.result() != http::status::no_content) cout << INDENT << target << " NOT loaded\n";
}

void HttpHelper::del(string const& target)
{
  auto s = tcp::socket{io};
  net::connect(endpoint_, s, yield_);

  auto req = http::request<http::string_body>{};
  req.method(http::verb::delete_);
  req.target(target);
  req.version(11);
  http::async_write(s, req, yield_);

  auto buf = beast::flat_static_buffer<1024>{};
  auto resp = http::response<http::string_body>{};
  http::async_read(s, buf, resp, yield_);
  assertTrue(resp.result() == http::status::no_content);
}

static auto genDirect() { return "{\"type\":\"direct\"}"; }

static auto genRoute() { return "{\"default\":\"direct\",\"rules\":[]}"; }

static auto readJson(string const& fn)
{
  auto doc = json::Document{};
  auto ifs = ifstream{fn};
  auto isw = json::IStreamWrapper{ifs};
  doc.ParseStream(isw);
  if (doc.HasParseError() || !doc.IsObject()) {
    cout << "Invalid JSON configuration\n";
    doc = json::Document{};
    doc.SetObject();
  }
  return doc;
}

static string toString(json::Value const& v)
{
  auto buf = json::StringBuffer{};
  auto writer = json::Writer<json::StringBuffer>{buf};
  v.Accept(writer);
  return buf.GetString();
}

template <typename StringRef>
static void loadSet(HttpHelper& helper, json::Value const& root, StringRef const& category)
{
  auto it = root.FindMember(category);
  if (it == root.MemberEnd() || !it->value.IsObject()) {
    cout << INDENT << category << " NOT loaded\n";
    return;
  }

  for (auto&& node : it->value.GetObject())
    helper.put("/"s + category + "/" + node.name.GetString(), toString(node.value));
}

static void load(HttpHelper& helper, string const& fn)
{
  cout << "Loading configuration: " << fn << "\n";
  auto json = readJson(fn);
  loadSet(helper, json, INGRESSES);
  loadSet(helper, json, EGRESSES);
  loadSet(helper, json, RULES);
  if (json.HasMember(ROUTE))
    helper.put("/"s + ROUTE, toString(json[ROUTE]));
  else
    cout << INDENT << ROUTE << " NOT loaded\n";
  cout << "Configuration " << fn << " loaded\n";
}

static void flush(HttpHelper& helper)
{
  helper.put("/"s + EGRESSES + "/direct", genDirect());
  helper.put("/"s + ROUTE, genRoute());

  for (auto&& rule : helper.get("/"s + RULES)) helper.del("/"s + RULES + "/" + rule);
  for (auto&& egress : helper.get("/"s + EGRESSES))
    if (egress != "direct") helper.del("/"s + EGRESSES + "/" + egress);
  for (auto&& ingress : helper.get("/"s + INGRESSES)) helper.del("/"s + INGRESSES + "/" + ingress);

  cout << "Configuration reset\n";
}

void run(string const& bind, uint16_t port, string const& fn, string const& mmdb)
{
  auto server = api::Server{io, mmdb.c_str()};
  server.listen(bind, port);

  net::spawn(
      io,
      [=](auto yield) {
        auto helper = HttpHelper{bind, port, yield};
        load(helper, fn);

#ifdef HAS_SIGNAL_H
        auto ss = asio::signal_set{io};
        while (true) {
          ss.add(SIGHUP);
          ss.async_wait(yield);
          flush(helper);
          load(helper, fn);
        }
#endif // HAS_SIGNAL_H
      },
      [](auto, auto) noexcept { io.stop(); });

  io.run();
}
