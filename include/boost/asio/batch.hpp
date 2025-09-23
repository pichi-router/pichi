#ifndef BOOST_ASIO_BATCH_HPP
#define BOOST_ASIO_BATCH_HPP

#include <algorithm>
#include <boost/asio/execution/bad_executor.hpp>
#include <boost/asio/execution/blocking.hpp>
#include <boost/asio/execution/context.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/outstanding_work.hpp>
#include <boost/asio/execution_context.hpp>
#include <cassert>
#include <concepts>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace boost::asio {

namespace detail {

template <typename Executor> struct IsBatch : public false_type {};

template <typename Executor>
concept BatchExecutor = IsBatch<Executor>::value;

class BatchService;

struct BatchImpl {
  using Ops = std::vector<std::packaged_task<void()>>;

  BatchService& service_;

  Ops  waitings_ = {};
  bool active_   = false;
  bool disabled_ = false;
};

class BatchService : public execution_context_service_base<BatchService> {
private:
  using Impl      = BatchImpl;
  using RawPtr    = Impl*;
  using SharedPtr = std::shared_ptr<Impl>;
  using WeakPtr   = std::weak_ptr<Impl>;
  using Queue     = std::list<std::pair<RawPtr, WeakPtr>>;

  void shutdown() noexcept override
  {
    auto guard = std::lock_guard{mutex_};
    stopped_   = true;
    queue_.clear();
  }

public:
  explicit BatchService(execution_context& ctx) : execution_context_service_base<BatchService>{ctx}
  {
  }

  SharedPtr create_impl()
  {
    auto guard = std::lock_guard{mutex_};

    if (stopped_) throw execution::bad_executor{};

    auto impl = SharedPtr{new Impl{.service_ = *this}, [](RawPtr impl) {
                            impl->service_.disable_impl(impl);
                            delete impl;
                          }};
    queue_.emplace_back(impl.get(), impl);
    impl->active_ = std::ranges::size(queue_) == 1;

    return impl;
  }

  void disable_impl(RawPtr impl)
  {
    if (impl == nullptr) return;

    auto ops = BatchImpl::Ops{};

    {
      auto head  = SharedPtr{nullptr};
      auto guard = std::lock_guard{mutex_};

      if (stopped_ || impl->disabled_) return;
      impl->disabled_ = true;

      std::erase_if(queue_, [=](auto&& item) { return item.first == impl; });
      if (!impl->active_) return;

      queue_.erase(std::ranges::begin(queue_), std::ranges::find_if(queue_, [&head](auto&& item) {
                     head = item.second.lock();
                     return head != nullptr;
                   }));
      if (head == nullptr) return;

      assert(!head->disabled_);
      assert(!head->active_);
      head->active_ = true;
      std::swap(ops, head->waitings_);
    }

    for (auto&& op : ops) op();
  }

  template <execution::executor Executor, std::invocable F>
  void schedule(Executor const& ex, SharedPtr& impl, F&& f)
  {
    {
      auto guard = std::lock_guard{mutex_};

      if (impl->disabled_) throw execution::bad_executor{};

      // TODO assert if impl is located in queue_

      if (!impl->active_) {
        // FIXME MSVC generates compilation error even F satisfies DECAY_COPY concept
        static_assert(std::copy_constructible<std::decay_t<F>>);
#ifdef _MSC_VER
        impl->waitings_.emplace_back([ex = prefer(
                                          require(ex, execution::blocking.never),
                                          execution::outstanding_work.tracked
                                      ),
                                      pf = std::make_shared<std::decay_t<F>>(std::forward<F>(f))](
                                     ) mutable { ex.execute(std::forward<F>(*pf)); });
#else   // _MSC_VER
        impl->waitings_.emplace_back([ex = prefer(
                                          require(ex, execution::blocking.never),
                                          execution::outstanding_work.tracked
                                      ),
                                      f = std::decay_t<F>{std::forward<F>(f)}]() mutable {
          ex.execute(std::forward<F>(f));
        });
#endif  // _MSC_VER
        return;
      }
    }
    ex.execute(std::forward<F>(f));
  }

private:
  std::mutex mutex_   = {};
  Queue      queue_   = {};
  bool       stopped_ = false;
};

}  // namespace detail

template <execution::executor Executor>
requires(!detail::BatchExecutor<Executor>)
class Batch {
private:
  using Service = detail::BatchService;
  using Impl    = std::shared_ptr<detail::BatchImpl>;

  Impl create_impl()
  {
    if constexpr (can_query_v<Executor const&, execution::context_t>)
      return use_service<Service>(boost::asio::query(inner_, execution::context)).create_impl();
    else
      return use_service<Service>(inner_.context()).create_impl();
  }

public:
  Batch() : Batch{Executor{}} {}

  template <typename OtherExecutor>
  requires(!detail::BatchExecutor<OtherExecutor> && std::convertible_to<OtherExecutor, Executor>)
  explicit Batch(OtherExecutor const& ex) : inner_{ex}, impl_{create_impl()}
  {
  }

  template <execution::executor OtherExecutor>
  requires(!detail::BatchExecutor<OtherExecutor> && std::convertible_to<OtherExecutor, Executor>)
  explicit Batch(OtherExecutor const& ex, Impl const& impl) : inner_{ex}, impl_{impl}
  {
  }

  template <typename OtherExecutor>
  requires std::convertible_to<OtherExecutor, Executor>
  Batch(Batch<OtherExecutor> const& other) : inner_{other.inner_}, impl_{other.impl_}
  {
  }

  template <typename OtherExecutor>
  requires std::convertible_to<OtherExecutor, Executor>
  Batch(Batch<OtherExecutor>&& other)
    : inner_{std::move(other.inner_)}, impl_{std::move(other.impl_)}
  {
  }

  template <typename OtherExecutor>
  requires std::convertible_to<OtherExecutor, Executor>
  Batch& operator=(Batch<OtherExecutor> const& other)
  {
    inner_ = other.inner_;
    impl_  = other.impl_;
    return *this;
  }

  template <typename OtherExecutor>
  requires std::convertible_to<OtherExecutor, Executor>
  Batch& operator=(Batch<OtherExecutor>&& other)
  {
    inner_ = std::move(other.inner_);
    impl_  = std::move(other.impl_);
    return *this;
  }

  template <typename AnyExecutor> bool operator==(AnyExecutor const& other) const noexcept
  {
    if constexpr (detail::BatchExecutor<AnyExecutor>)
      return impl_ == other.impl_;
    else
      return false;
  }

  template <typename Property>
  requires can_query_v<Executor const&, Property>
  decltype(auto) query(Property const& prop) const
      noexcept(is_nothrow_query_v<Executor const&, Property>)
  {
    if constexpr (std::convertible_to<Property, execution::blocking_t>) {
      auto rt = boost::asio::query(inner_, prop);
      return rt == execution::blocking.always ? execution::blocking.possibly : rt;
    }
    else
      return boost::asio::query(inner_, prop);
  }

  template <typename Property>
  requires can_require_v<Executor const&, Property>
  auto require(Property const& prop) const noexcept(is_nothrow_require_v<Executor const&, Property>)
  {
    return Batch<std::decay_t<require_result_t<Executor const&, Property>>>{
        boost::asio::require(inner_, prop),
        impl_
    };
  }

  template <typename Property>
  requires can_prefer_v<Executor const&, Property>
  auto prefer(Property const& prop) const noexcept(is_nothrow_prefer_v<Executor const&, Property>)
  {
    return Batch<std::decay_t<prefer_result_t<Executor const&, Property>>>{
        boost::asio::prefer(inner_, prop),
        impl_
    };
  }

  template <std::invocable F> void execute(F&& f) const
  {
    impl_->service_.schedule(inner_, impl_, std::forward<F>(f));
  }

  void     disable() { impl_->service_.disable_impl(impl_.get()); }
  Executor get_inner_executor() const noexcept { return inner_; }

private:
  Executor     inner_;
  mutable Impl impl_;
};

template <execution::executor Executor> using batch = Batch<Executor>;

template <execution::executor Executor> auto make_batch(Executor const& ex)
{
  return batch<Executor>{ex};
}

template <typename Context>
requires std::derived_from<Context, execution_context>
auto make_batch(Context& ctx)
{
  return make_batch(ctx.get_executor());
}

namespace detail {

template <execution::executor Executor> struct IsBatch<Batch<Executor>> : public std::true_type {};

}  // namespace detail

}  // namespace boost::asio

#endif  // BOOST_ASIO_BATCH_HPP
