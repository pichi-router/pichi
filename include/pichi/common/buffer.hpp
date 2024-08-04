#ifndef PICHI_COMMON_BUFFER_HPP
#define PICHI_COMMON_BUFFER_HPP

#include <concepts>
#include <ranges>
#include <span>
#include <stdint.h>
#include <type_traits>

namespace pichi {

template <typename T>
concept VoidPointer = std::same_as<void, std::remove_const_t<std::remove_pointer_t<T>>>;

template <typename Pod>
concept PodType = std::is_standard_layout_v<Pod> && std::is_trivial_v<Pod>;

template <typename R>
concept ConstContiguousRange = std::ranges::contiguous_range<R> && requires(R r)
{
  {
    std::ranges::data(r)
    } -> std::same_as<decltype(std::ranges::cdata(r))>;
};

template <typename R>
concept CStyleRange = requires(R r)
{
  {
    r.data()
    } -> VoidPointer;
  {
    r.size()
    } -> std::same_as<size_t>;
};

template <PodType Pod> struct Buffer : public std::span<Pod> {
  template <typename... Args>
  requires std::constructible_from<std::span<Pod>, Args...>
  constexpr Buffer(Args&&... args) : std::span<Pod>{std::forward<Args>(args)...} {}

  template <std::ranges::contiguous_range R>
  requires(sizeof(std::ranges::range_value_t<R>) == sizeof(Pod) &&
           (std::is_const_v<Pod> || !ConstContiguousRange<R>)) constexpr Buffer(R&& r)
    : Buffer{reinterpret_cast<Pod*>(std::ranges::data(r)), std::ranges::size(r)}
  {
  }

  template <std::ranges::contiguous_range R>
  requires(std::constructible_from<Buffer, R>) constexpr Buffer(R&& r, size_t n)
    : Buffer{reinterpret_cast<Pod*>(std::ranges::data(r)), n}
  {
  }

  template <CStyleRange R> constexpr Buffer(R&& r) : Buffer{static_cast<Pod*>(r.data()), r.size()}
  {
  }

  template <CStyleRange R>
  constexpr Buffer(R&& r, size_t n) : Buffer{static_cast<Pod*>(r.data()), n}
  {
  }

  Buffer& operator+=(size_t n)
  {
    auto it = std::ranges::begin(*this);
    auto d = std::ranges::advance(it, n, std::ranges::end(*this));
    *this = Buffer{it, d == 0 ? std::ranges::size(*this) - n : 0};
    return *this;
  }
};

template <PodType Pod> Buffer<Pod> operator+(Buffer<Pod> const& b, size_t n)
{
  auto ret = b;
  ret += n;
  return ret;
}

template <PodType Pod> Buffer<Pod> operator+(size_t n, Buffer<Pod> const& b) { return b + n; }

using ConstBuffer = Buffer<uint8_t const>;
using MutableBuffer = Buffer<uint8_t>;

}  // namespace pichi

#endif  // PICHI_COMMON_BUFFER_HPP
