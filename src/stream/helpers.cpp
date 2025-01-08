#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/connect.hpp>
#include <pichi/stream/helpers.hpp>

namespace asio = boost::asio;
namespace ip   = asio::ip;

namespace pichi::stream {

Awaitable<void> close(ip::tcp::socket& s)
{
  s.close();
  co_return;
}

Awaitable<void> connect(ip::tcp::socket& s, Endpoint const& peer)
{
  if (peer.type_ == EndpointType::DOMAIN_NAME) {
    co_await asio::async_connect(
        s,
        co_await ip::tcp::resolver{s.get_executor()}
            .async_resolve(peer.host_, std::to_string(peer.port_), asio::use_awaitable),
        asio::use_awaitable
    );
  }
  else {
    co_await s.async_connect({ip::make_address(peer.host_), peer.port_}, asio::use_awaitable);
  }
}

Awaitable<void> accept(ip::tcp::socket&) { co_return; }

}  // namespace pichi::stream
