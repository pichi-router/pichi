#ifndef PICHI_STREAM_COMPLETER_HPP
#define PICHI_STREAM_COMPLETER_HPP

#include <boost/asio/associated_executor.hpp>
#include <concepts>
#include <exception>

namespace pichi::stream {

template <typename CompletionHandler> class Completer {
private:
  using CompletionExecutor = boost::asio::associated_executor<CompletionHandler>::type;

public:
  using executor_type = CompletionExecutor;

  explicit Completer(CompletionHandler&& h)
    : h_{std::forward<CompletionHandler>(h)}, ex_{boost::asio::get_associated_executor(h_)}
  {
  }

  executor_type const& get_executor() const noexcept { return ex_; }

  template <typename... Args>
  requires std::invocable<CompletionHandler, boost::system::error_code, Args...>
  void operator()(std::exception_ptr ep, Args&&... args)
  {
    try {
      if (ep) std::rethrow_exception(ep);
      std::invoke(h_, boost::system::error_code{}, std::forward<Args>(args)...);
    }
    catch (boost::system::system_error const& e) {
      std::invoke(h_, e.code(), std::forward<Args>(args)...);
    }
    catch (...) {
      // TODO log this unexpected exception
      std::terminate();
    }
  }

private:
  CompletionHandler  h_;
  CompletionExecutor ex_;
};

}  // namespace pichi::stream

#endif  // PICHI_STREAM_COMPLETER_HPP
