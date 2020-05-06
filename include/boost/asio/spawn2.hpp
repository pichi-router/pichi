#ifndef BOOST_ASIO_SPAWN2_HPP
#define BOOST_ASIO_SPAWN2_HPP

#include <boost/asio/associated_executor.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/coroutine2/coroutine.hpp>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>

namespace boost::asio {

namespace detail {

template <typename T> class SpawnHandler;
template <typename T> class AsyncResult;
class YieldContext;

using ErrorCode = boost::system::error_code;
using Pull = typename boost::coroutines2::coroutine<void>::pull_type;
using Push = typename boost::coroutines2::coroutine<void>::push_type;
using DefaultAllocator = boost::coroutines2::default_stack;

} // namespace detail

using yield_context = detail::YieldContext;

template <typename T> struct associated_executor<detail::SpawnHandler<T>>;
template <> struct associated_executor<yield_context>;

namespace detail {

enum class YieldState { INIT, PUSHED, PULLED };

class YieldContext {
private:
  friend struct associated_executor<YieldContext>;
  template <typename T> friend class SpawnHandler;
  template <typename T> friend class AsyncResult;

public:
  YieldContext(executor const& ex, YieldState& state, Push& push, Pull& pull)
    : ex_{ex}, state_{state}, push_{push}, pull_{pull}
  {
  }

  YieldContext operator[](ErrorCode& ec)
  {
    auto ctx = *this;
    ctx.ec_ = &ec;
    return ctx;
  }

private:
  /*
   * Here's the diagram of yield FSM:
   *
   *   +--------+  no-op  +--------+  pull  +--------+
   *   |        +--------->        +-------->        |
   *   | PUSHED |         |  INIT  |        | PULLED |
   *   |        <---------+        <--------+        |
   *   +--------+  no-op  +-+----^-+  push  +--------+
   *                        |    |
   *                        +----+
   *                        no-op
   */
  void yield(YieldState next)
  {
    switch (state_) {
    case YieldState::INIT:
      state_ = next;
      if (next == YieldState::PULLED) pull_();
      break;
    case YieldState::PUSHED:
      assert(next == YieldState::PULLED);
      state_ = YieldState::INIT;
      break;
    default:
      assert(next == YieldState::PUSHED);
      state_ = YieldState::INIT;
      push_();
      break;
    }
  }

  ErrorCode* ec_ = nullptr;
  executor ex_;
  YieldState& state_;
  Push& push_;
  Pull& pull_;
};

template <typename Executor, typename Function, typename StackAllocator>
class SpawnContext
  : public std::enable_shared_from_this<SpawnContext<Executor, Function, StackAllocator>> {
public:
  SpawnContext(strand<Executor> const& s, Function&& f, StackAllocator&& alloc)
    : ex_{s}, f_{std::forward<Function>(f)}, alloc_{std::forward<StackAllocator>(alloc)}
  {
  }
  // Copy/Move constructors and assignments are implicitly deleted
  ~SpawnContext()
  {
    // The stack memory corresponding to pPush_ will not be really deallocated,
    //   even if (*pPush_) is destructed, unless (*pPush_) is moved out of
    //   its own stack and destructed outside.
    post(ex_, [push = std::move(*push_)]() {});
  }

  void start()
  {
    assert(!push_.has_value());
    push_ = std::make_optional<Push>(alloc_, [self = this->shared_from_this(), this](auto&& pull) {
      f_(YieldContext{ex_, state_, *push_, pull});
    });
    (*push_)();
  }

private:
  YieldState state_ = YieldState::INIT;
  executor ex_;
  std::decay_t<Function> f_;
  std::decay_t<StackAllocator> alloc_;
  std::optional<Push> push_;
};

template <typename T> struct AsyncResultData {
  static_assert(std::is_same_v<T, std::decay_t<T>>);
  ErrorCode ec_ = {};
  std::optional<T> t_ = {};
};

template <> struct AsyncResultData<void> {
  ErrorCode ec_ = {};
};

template <typename T> class SpawnHandler {
private:
  friend class AsyncResult<T>;
  friend struct associated_executor<detail::SpawnHandler<T>>;

public:
  SpawnHandler(YieldContext yield) : yield_{yield} {}

  template <typename... Args> void operator()(ErrorCode const& ec, Args&&... args)
  {
    assert(data_ != nullptr);
    data_->ec_ = ec;
    if constexpr (!std::is_same_v<T, void>) {
      static_assert(std::is_constructible_v<T, Args&&...>);
      data_->t_.emplace(std::forward<Args>(args)...);
    }
    yield_.yield(YieldState::PUSHED);
  }

private:
  YieldContext yield_;
  AsyncResultData<T>* data_ = nullptr;
};

template <typename T> class AsyncResult {
public:
  using return_type = T;
  using completion_handler_type = SpawnHandler<T>;

  AsyncResult(SpawnHandler<T>& h) : yield_{h.yield_}
  {
    h.data_ = &data_;
    yield_.yield(YieldState::INIT);
  }

  T get()
  {
    yield_.yield(YieldState::PULLED);
    if (yield_.ec_ != nullptr)
      *yield_.ec_ = data_.ec_;
    else if (data_.ec_)
      throw_error(data_.ec_);
    if constexpr (std::is_same_v<T, void>)
      return;
    else if constexpr (std::is_move_constructible_v<T>)
      return std::move(*data_.t_);
    else
      return *data_.t_;
  }

private:
  YieldContext yield_;
  AsyncResultData<T> data_ = {};
};

} // namespace detail

template <typename R> struct async_result<yield_context, R()> : public detail::AsyncResult<void> {
  async_result(typename detail::AsyncResult<void>::completion_handler_type& h)
    : detail::AsyncResult<void>{h}
  {
  }
};

template <typename R, typename E>
struct async_result<yield_context, R(E)> : public detail::AsyncResult<void> {
  static_assert(std::is_same_v<std::decay_t<E>, detail::ErrorCode>);
  async_result(typename detail::AsyncResult<void>::completion_handler_type& h)
    : detail::AsyncResult<void>{h}
  {
  }
};

template <typename R, typename E, typename T>
struct async_result<yield_context, R(E, T)> : public detail::AsyncResult<std::decay_t<T>> {
  static_assert(std::is_same_v<std::decay_t<E>, detail::ErrorCode>);
  async_result(typename detail::AsyncResult<std::decay_t<T>>::completion_handler_type& h)
    : detail::AsyncResult<std::decay_t<T>>{h}
  {
  }
};

template <typename T> bool asio_handler_is_continuation(detail::SpawnHandler<T>*) { return true; }

template <typename F, typename T> void asio_handler_invoke(F&& f, detail::SpawnHandler<T>*)
{
  std::invoke(std::forward<F>(f));
}

template <typename T> struct associated_executor<detail::SpawnHandler<T>> {
  using type = executor;

  static type get(detail::SpawnHandler<T> const& h,
                  executor const& = executor{}) BOOST_ASIO_NOEXCEPT
  {
    return get_associated_executor(h.yield_);
  }
};

template <> struct associated_executor<yield_context> {
  using type = executor;

  static type get(yield_context const& yield, executor const& = executor{}) BOOST_ASIO_NOEXCEPT
  {
    return yield.ex_;
  }
};

template <typename Function, typename Executor, typename StackAllocator = detail::DefaultAllocator>
void spawn(strand<Executor> const& s, Function&& function,
           StackAllocator&& alloc = StackAllocator{})
{
  static_assert(std::is_invocable_v<std::decay_t<Function>, yield_context>);
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

template <typename Function, typename StackAllocator = detail::DefaultAllocator>
void spawn(yield_context yield, Function&& function, StackAllocator&& alloc = StackAllocator{})
{
  boost::asio::spawn(get_associated_executor(yield), std::forward<Function>(function),
                     std::forward<StackAllocator>(alloc));
}

} // namespace boost::asio

#endif // BOOST_ASIO_SPAWN2_HPP
