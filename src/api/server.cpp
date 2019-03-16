#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <iostream>
#include <memory>
#include <pichi/api/server.hpp>
#include <pichi/api/vos.hpp>
#include <pichi/asserts.hpp>
#include <pichi/net/spawn.hpp>

using namespace std;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ip = asio::ip;
namespace sys = boost::system;
using tcp = ip::tcp;

namespace pichi::api {

Server::Server(asio::io_context& io, char const* fn)
  : strand_{io}, router_{fn}, egresses_{},
    ingresses_{strand_, router_, egresses_}, rest_{ingresses_, egresses_, router_}
{
}

void Server::listen(string_view address, uint16_t port)
{
  net::spawn(strand_, [a = tcp::acceptor{strand_.context(), {ip::make_address(address), port}},
                       this](auto yield) mutable {
    while (true) {
      auto s = make_shared<tcp::socket>(a.async_accept(yield));
      net::spawn(
          strand_,
          [this, s](auto yield) mutable {
            auto buf = beast::flat_buffer{};
            auto req = Rest::Request{};
            http::async_read(*s, buf, req, yield);

            auto resp = rest_.handle(req);
            http::async_write(*s, resp, yield);
          },
          [s](auto eptr, auto yield) noexcept {
            auto ec = sys::error_code{};
            auto resp = Rest::errorResponse(eptr);
            http::async_write(*s, resp, yield[ec]);
            if (ec) cout << "Ignoring HTTP error: " << ec.message() << endl;
          });
    }
  });
}

} // namespace pichi::api
