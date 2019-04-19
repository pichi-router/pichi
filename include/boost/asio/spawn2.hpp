#ifndef BOOST_ASIO_SPAWN2_HPP
#define BOOST_ASIO_SPAWN2_HPP

#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/coroutine2/all.hpp>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>

namespace boost::asio {

namespace detail {

using ErrorCode = boost::system::error_code;
using Pull = typename boost::coroutines2::coroutine<ErrorCode>::pull_type;
using Push = typename boost::coroutines2::coroutine<ErrorCode>::push_type;
using DefaultAllocator = boost::coroutines2::default_stack;

class YieldContext {
public:
  YieldContext(Push& push, Pull& pull) : push_{push}, pull_{pull} {}

  void push(ErrorCode const& ec) { push_(ec); }

  void pull()
  {
    pull_();
    auto ec = pull_.get();
    if (ec_ == nullptr)
      throw_error(ec);
    else
      *ec_ = ec;
  }

  YieldContext operator[](ErrorCode& ec)
  {
    auto ctx = *this;
    ctx.ec_ = &ec;
    return ctx;
  }

private:
  Push& push_;
  Pull& pull_;
  ErrorCode* ec_ = nullptr;
};

template <typename Executor, typename Function, typename StackAllocator>
class SpawnContext
  : public std::enable_shared_from_this<SpawnContext<Executor, Function, StackAllocator>> {
public:
  SpawnContext(strand<Executor> const& s, Function&& f, StackAllocator&& alloc)
    : s_{s}, f_{std::forward<Function>(f)}, alloc_{std::forward<StackAllocator>(alloc)}
  {
  }
  // Copy/Move constructors and assignments are implicitly deleted
  ~SpawnContext()
  {
    // The stack memory corresponding to pPush_ will not be really deallocated,
    //   even if (*pPush_) is destructed, unless (*pPush_) is moved out of
    //   its own stack and destructed outside.
    post(s_, [push = std::move(*push_)]() {});
  }

  void start()
  {
    assert(!push_.has_value());
    push_ = std::make_optional<Push>(alloc_, [self = this->shared_from_this(), this](auto&& pull) {
      f_(YieldContext{*push_, pull});
    });
    (*push_)({});
  }

private:
  strand<Executor> s_;
  std::decay_t<Function> f_;
  std::decay_t<StackAllocator> alloc_;
  std::optional<Push> push_;
};

template <typename T> struct SpawnHandler {
  static_assert(std::is_same_v<T, std::decay_t<T>>);

  SpawnHandler(YieldContext yield) : yield_{yield} {}

  template <typename... Args> void operator()(ErrorCode const& ec, Args&&... args)
  {
    static_assert(std::is_constructible_v<T, Args&&...>);
    assert(t_ != nullptr);
    t_->emplace(std::forward<Args>(args)...);
    yield_.push(ec);
  }

  YieldContext yield_;
  std::optional<T>* t_ = nullptr;
};

template <> struct SpawnHandler<void> {
  SpawnHandler(YieldContext yield) : yield_{yield} {}

  void operator()(ErrorCode const& ec = {}) { yield_.push(ec); }

  YieldContext yield_;
};

} // namespace detail

template <typename R> struct async_result<detail::YieldContext, R()> {
  using return_type = void;
  using completion_handler_type = detail::SpawnHandler<void>;

  async_result(completion_handler_type& h) : yield_{h.yield_} {}
  void get() { yield_.pull(); }

  detail::YieldContext yield_;
};

template <typename R, typename E> struct async_result<detail::YieldContext, R(E)> {
  static_assert(std::is_same_v<std::decay_t<E>, detail::ErrorCode>);

  using return_type = void;
  using completion_handler_type = detail::SpawnHandler<void>;

  async_result(completion_handler_type& h) : yield_{h.yield_} {}
  void get() { yield_.pull(); }

  detail::YieldContext yield_;
};

template <typename R, typename E, typename T> struct async_result<detail::YieldContext, R(E, T)> {
  static_assert(std::is_same_v<std::decay_t<E>, detail::ErrorCode>);

  using return_type = std::decay_t<T>;
  using completion_handler_type = detail::SpawnHandler<return_type>;

  async_result(completion_handler_type& h) : yield_{h.yield_}, t_{} { h.t_ = &t_; }
  return_type get()
  {
    yield_.pull();
    // TODO It's supposed to be noexcept here
    assert(t_.has_value());
    if constexpr (std::is_move_constructible_v<return_type>)
      return std::move(*t_);
    else
      return *t_;
  }

  detail::YieldContext yield_;
  std::optional<return_type> t_;
};

template <typename T> bool asio_handler_is_continuation(detail::SpawnHandler<T>*) { return true; }

template <typename F, typename T> void asio_handler_invoke(F&& f, detail::SpawnHandler<T>*)
{
  std::invoke(std::forward<F>(f));
}

template <typename Function, typename Executor, typename StackAllocator = detail::DefaultAllocator>
void spawn(strand<Executor> const& s, Function&& function,
           StackAllocator&& alloc = StackAllocator{})
{
  static_assert(std::is_invocable_v<std::decay_t<Function>, detail::YieldContext>);
  using Context = detail::SpawnContext<Executor, Function, StackAllocator>;
  dispatch(s, [pCtx = std::make_shared<Context>(s, std::forward<Function>(function),
                                                std::forward<StackAllocator>(alloc))]() {
    pCtx->start();
  });
}

template <typename Function, typename Executor, typename StackAllocator = detail::DefaultAllocator>
void spawn(Executor const& ex, Function&& function, StackAllocator&& alloc = StackAllocator{},
           std::enable_if_t<is_executor<Executor>::value>* = nullptr)
{
  boost::asio::spawn(strand<Executor>{ex}, std::forward<Function>(function),
                     std::forward<StackAllocator>(alloc));
}

template <typename Function, typename ExecutorContext,
          typename StackAllocator = detail::DefaultAllocator>
void spawn(ExecutorContext& ctx, Function&& function, StackAllocator&& alloc = StackAllocator{},
           std::enable_if_t<std::is_convertible_v<ExecutorContext&, execution_context&>>* = nullptr)
{
  boost::asio::spawn(ctx.get_executor(), std::forward<Function>(function),
                     std::forward<StackAllocator>(alloc));
}

using yield_context = detail::YieldContext;

} // namespace boost::asio

#endif // BOOST_ASIO_SPAWN2_HPP
