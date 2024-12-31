#include <pichi/common/config.hpp>
// Include config.hpp first
#include <pichi/adapter/tcp/direct.hpp>
#include <pichi/stream/helpers.hpp>

using namespace std;
namespace asio = boost::asio;
namespace sys  = boost::system;

namespace pichi::adapter::tcp {

Awaitable<void> Direct::connect(Endpoint const& peer) { co_await stream::connect(socket_, peer); }

Awaitable<size_t> Direct::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(socket_, buf);
}

Awaitable<void> Direct::send(ConstBuffer buf) { co_await stream::write(socket_, buf); }

Awaitable<void> Direct::close() { co_await stream::close(socket_); }

}  // namespace pichi::adapter::tcp
