#ifndef PICHI_STREAM_STREAM_HPP
#define PICHI_STREAM_STREAM_HPP

#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <functional>
#include <type_traits>
#include <utility>

namespace boost::asio {

namespace detail {

template <typename Stream, typename Iterator, typename ConnectHandler>
struct IteratorConnectOperator {
  IteratorConnectOperator(Stream& s, Iterator it, ConnectHandler const& h) : s_{s}, it_{it}, h_{h}
  {
  }

  void operator()(boost::system::error_code ec)
  {
    if (!ec) {
      post(get_associated_executor(h_), [h = h_, ec]() mutable { std::invoke(h, ec); });
      return;
    }
    if (++it_ == Iterator{}) {
      post(get_associated_executor(h_), [h = h_, ec]() mutable { std::invoke(h, ec); });
      return;
    }
    s_.close(ec);
    s_.async_connect(*it_, IteratorConnectOperator(s_, it_, h_));
  }

  Stream& s_;
  Iterator it_;
  ConnectHandler h_;
};

}  // namespace detail

template <typename Stream, typename Iterator, typename ConnectHandler>
bool asio_handler_is_continuation(
    detail::IteratorConnectOperator<Stream, Iterator, ConnectHandler>*)
{
  return true;
}

template <typename F, typename Stream, typename Iterator, typename ConnectHandler>
void asio_handler_invoke(F&& f, detail::IteratorConnectOperator<Stream, Iterator, ConnectHandler>*)
{
  std::invoke(std::forward<F>(f));
}

template <typename Stream, typename Iterator, typename ConnectHandler>
struct associated_executor<detail::IteratorConnectOperator<Stream, Iterator, ConnectHandler>> {
  using type = executor;

  static type get(detail::IteratorConnectOperator<Stream, Iterator, ConnectHandler> const& h,
                  executor const& = executor{}) BOOST_ASIO_NOEXCEPT
  {
    return get_associated_executor(h.h_);
  }
};

// TODO Boost.Asio provides async_initiate function template from 1.70.0.
template <typename Signature, typename Initiation, typename CompletionToken, typename... Args>
auto asyncInitiate(Initiation&& initiation, CompletionToken&& token, Args&&... args)
{
  auto t = std::forward<CompletionToken>(token);
  auto init = async_completion<std::decay_t<CompletionToken>, Signature>{t};
  std::invoke(std::forward<Initiation>(initiation), init.completion_handler,
              std::forward<Args>(args)...);
  return init.result.get();
}

/*
 *  boost::asio::async_connect only supports basic_socket, so asyncConnect is
 * intended to support all the stream classes with async_connect member function
 * template.
 */
template <typename Stream, typename Results, typename ConnectHandler>
auto asyncConnect(Stream& stream, Results results, ConnectHandler&& handler)
{
  // FIXME the life term of results should be extended until all of the
  // async-ops are accomplished,
  //   but according to the implementation of ResolveResults::iterator, the
  //   iterator will fulfill the extension. So, keep it this way here.
  return asyncInitiate<void(boost::system::error_code)>(
      [](auto&& h, auto& stream, auto results) {
        using Handler = decltype(h);
        static_assert(std::is_invocable_v<Handler, boost::system::error_code const&>);
        if (results.empty()) {
          post(get_associated_executor(h),
               [h = std::forward<Handler>(h)]() mutable { std::invoke(h, error::host_not_found); });
          return;
        }
        auto first = std::cbegin(results);
        stream.async_connect(*first, detail::IteratorConnectOperator{stream, first, h});
      },
      std::forward<ConnectHandler>(handler), stream, std::move(results));
}

}  // namespace boost::asio

#endif  // PICHI_STREAM_STREAM_HPP
