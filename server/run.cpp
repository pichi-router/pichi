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

static asio::io_context io{1};

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

static void loadValue(net::Endpoint const& server, asio::yield_context yield,
                      json::Value const& value, string const& target)
{
  auto socket = tcp::socket{io};
  net::connect(server, socket, yield);

  auto req = http::request<http::string_body>{};
  req.target(target);
  req.method(http::verb::put);
  req.body() = toString(value);
  req.prepare_payload();
  http::async_write(socket, req, yield);

  auto buf = beast::flat_static_buffer<1024>{};
  auto resp = http::response<http::string_body>{};
  http::async_read(socket, buf, resp, yield);
  if (resp.result() == http::status::no_content)
    cout << target << " loaded.\n";
  else
    cout << target << " NOT loaded\n";
}

template <typename StringRef>
static void loadSet(net::Endpoint const& endpoint, asio::yield_context yield,
                    json::Value const& root, StringRef const& category)
{
  auto it = root.FindMember(category);
  if (it == root.MemberEnd() || !it->value.IsObject()) {
    cout << category << " NOT loaded\n";
    return;
  }

  for (auto&& node : it->value.GetObject())
    loadValue(endpoint, yield, node.value, "/"s + category + "/" + node.name.GetString());
}

static void loadJson(net::Endpoint const& endpoint, json::Value const& v, asio::yield_context yield)
{
  loadSet(endpoint, yield, v, INGRESSES);
  loadSet(endpoint, yield, v, EGRESSES);
  loadSet(endpoint, yield, v, RULES);
  if (v.HasMember(ROUTE))
    loadValue(endpoint, yield, v[ROUTE], "/"s + ROUTE);
  else
    cout << ROUTE << " NOT loaded\n";
}

void run(string const& bind, uint16_t port, string const& json, string const& mmdb)
{
  auto server = api::Server{io, mmdb.c_str()};
  server.listen(bind, port);

  net::spawn(io, [ep = net::makeEndpoint(bind, port), v = readJson(json)](auto yield) {
    loadJson(ep, v, yield);
  });

  io.run();
}
