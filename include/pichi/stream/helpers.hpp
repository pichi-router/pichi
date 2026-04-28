#ifndef PICHI_STREAM_HELPERS_HPP
#define PICHI_STREAM_HELPERS_HPP

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/stream/completer.hpp>
#include <pichi/stream/concepts.hpp>
#include <variant>

namespace pichi::stream {

template <Closable Socket> Awaitable<void> close(Socket& s)
{
  s.close();
  co_return;
}

template <typename Stream>
requires(Layered<Stream> && Shutdownable<Stream>)
Awaitable<void> close(Stream& stream)
{
  co_await stream.async_shutdown(boost::asio::use_awaitable);
  co_await close(stream.next_layer());
}

template <typename... Streams> Awaitable<void> close(std::variant<Streams...>& streams)
{
  co_await std::visit([](auto&& stream) { return close(stream); }, streams);
}

template <AsyncSocket Socket> Awaitable<void> connect(Socket& s, Endpoint const& peer)
{
  if constexpr (!std::same_as<Socket, boost::asio::ip::tcp::socket>)
    co_await s.async_connect({}, boost::asio::use_awaitable);
  else if (peer.type_ == EndpointType::DOMAIN_NAME)
    co_await boost::asio::async_connect(
        s,
        co_await boost::asio::ip::tcp::resolver{s.get_executor()}
            .async_resolve(peer.host_, std::to_string(peer.port_), boost::asio::use_awaitable)
    );

  else
    co_await s.async_connect(
        {boost::asio::ip::make_address(peer.host_), peer.port_},
        boost::asio::use_awaitable
    );
}

template <AsyncStream Stream>
requires(Connectable<Stream>)
Awaitable<void> connect(Stream& stream, Endpoint const& peer)
{
  co_await stream.async_connect(peer, boost::asio::use_awaitable);
}

template <AsyncStream Stream>
requires(Handshakable<Stream>)
Awaitable<void> connect(Stream& stream, Endpoint const& peer)
{
  co_await connect(stream.next_layer(), peer);
  co_await stream.async_handshake(boost::asio::use_awaitable);
}

template <typename... Streams>
Awaitable<void> connect(std::variant<Streams...>& streams, Endpoint const& peer)
{
  co_await std::visit([&](auto&& stream) { return connect(stream, peer); }, streams);
}

template <AsyncSocket Socket> Awaitable<void> accept(Socket&) { co_return; }

template <AsyncStream Stream>
requires(Acceptable<Stream>)
Awaitable<void> accept(Stream& stream)
{
  co_await accept(stream.next_layer());
  co_await stream.async_accept(boost::asio::use_awaitable);
}

template <typename... Streams> Awaitable<void> accept(std::variant<Streams...>& streams)
{
  co_await std::visit([](auto&& stream) { return accept(stream); }, streams);
}

Awaitable<void> read(AsyncReadable auto& stream, MutableBuffer buf)
{
  co_await boost::asio::async_read(
      stream,
      boost::asio::buffer(buf.data(), buf.size()),
      boost::asio::use_awaitable
  );
}

template <AsyncReadable... Streams>
Awaitable<void> read(std::variant<Streams...>& streams, MutableBuffer buf)
{
  co_await std::visit([=](auto&& stream) { return read(stream, buf); }, streams);
}

Awaitable<size_t> read_some(AsyncReadable auto& stream, MutableBuffer buf)
{
  co_return co_await stream.async_read_some(
      boost::asio::buffer(buf.data(), buf.size()),
      boost::asio::use_awaitable
  );
}

template <AsyncReadable... Streams>
Awaitable<size_t> read_some(std::variant<Streams...>& streams, MutableBuffer buf)
{
  co_return co_await std::visit([=](auto&& stream) { return read_some(stream, buf); }, streams);
}

Awaitable<void> write(AsyncWritable auto& stream, ConstBuffer buf)
{
  co_await boost::asio::async_write(stream, boost::asio::buffer(buf), boost::asio::use_awaitable);
}

template <AsyncWritable... Streams>
Awaitable<void> write(std::variant<Streams...>& streams, ConstBuffer buf)
{
  co_await std::visit([=](auto&& stream) { return write(stream, buf); }, streams);
}

Awaitable<size_t> write_some(AsyncWritable auto& stream, ConstBuffer buf)
{
  co_return co_await stream.async_write_some(boost::asio::buffer(buf), boost::asio::use_awaitable);
}

template <AsyncWritable... Streams>
Awaitable<size_t> write_some(std::variant<Streams...>& streams, ConstBuffer buf)
{
  co_return co_await std::visit([=](auto&& stream) { return write_some(stream, buf); }, streams);
}

template <
    typename Signature, typename CompletionToken, boost::asio::execution::executor Executor,
    typename Func, typename... Args>
requires(std::invocable<Func, Args...>)
auto async_initiate(Executor const& ex, CompletionToken&& token, Func&& f, Args&&... args)
{
  return boost::asio::async_initiate<CompletionToken, Signature>(
      [ex](auto&& h, auto&& f, auto&&... args) {
        boost::asio::co_spawn(
            ex,
            std::invoke(std::forward<Func>(f), std::forward<Args>(args)...),
            Completer{std::move(h)}
        );
      },
      token,
      std::forward<Func>(f),
      std::forward<Args>(args)...
  );
}

template <typename Signature, typename CompletionToken, typename... Args>
requires(!boost::asio::execution::executor<CompletionToken>)
auto async_initiate(CompletionToken&& token, Args&&... args)
{
  return boost::asio::async_initiate<CompletionToken, Signature>(
      [](auto&& h, auto&&... args) {
        auto ex = boost::asio::get_associated_executor(h);
        boost::asio::dispatch(
            ex,
            [h = std::move(h), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
              std::apply(h, std::move(args));
            }
        );
      },
      token,
      std::forward<Args>(args)...
  );
}

}  // namespace pichi::stream

#endif  // PICHI_STREAM_HELPERS_HPP
