#ifndef PICHI_COMMON_BUFFER_HPP
#define PICHI_COMMON_BUFFER_HPP

#include <cassert>
#include <concepts>
#include <stddef.h>
#include <type_traits>

namespace pichi {

template <typename Pod>
concept PodType = std::is_standard_layout_v<Pod> && std::is_trivial_v<Pod>;

template <typename C>
concept PodContainer = requires(C c) {
  requires std::is_pointer_v<decltype(c.data())>;
  { c.size() } -> std::convertible_to<size_t>;
};

template <PodContainer C, typename P = decltype(std::declval<C>().data())>
using PodContainerValueT = std::remove_pointer_t<P>;

template <PodType Pod> class Buffer;

template <PodType Pod> Buffer<Pod> operator+(Buffer<Pod> const&, size_t);
template <PodType Pod> Buffer<Pod> operator+(size_t, Buffer<Pod> const&);

// TODO avoid lots of constructors if conditional explicit is supported
template <PodType Pod> class Buffer {
private:
  enum class ConversionType { IMPLICIT, STATIC, EXPLICIT, UNKNOWN };

  template <PodContainer C> static constexpr ConversionType detect()
  {
    using ValueT = PodContainerValueT<C>;
    if constexpr (std::is_const_v<ValueT> && !std::is_const_v<Pod>)
      return ConversionType::UNKNOWN;
    else if constexpr (std::is_convertible_v<ValueT*, Pod*>)
      return ConversionType::IMPLICIT;
    else if constexpr (std::is_same_v<std::decay_t<ValueT>, void>)
      return ConversionType::STATIC;
    else if constexpr (sizeof(ValueT) == sizeof(Pod))
      return ConversionType::EXPLICIT;
    else
      return ConversionType::UNKNOWN;
  }

public:
  Buffer() = default;

  Buffer(Pod* data, size_t size) : data_{data}, size_{size} {}

  template <size_t N> Buffer(Pod (&data)[N]) : Buffer{std::decay_t<decltype(data)>(&data), N} {}

  template <typename Container, ConversionType t = detect<Container>()>
    requires(t != ConversionType::UNKNOWN)
  explicit(t == ConversionType::EXPLICIT) Buffer(Container&& c, size_t size)
    : Buffer{reinterpret_cast<Pod*>(c.data()), size}
  {
    assert(size <= c.size());
  }

  template <typename Container, ConversionType t = detect<Container>()>
    requires(t != ConversionType::UNKNOWN)
  explicit(t == ConversionType::EXPLICIT) Buffer(Container&& c)
    : Buffer{std::forward<Container>(c), c.size()}
  {
  }

public:
  Pod* data() const { return data_; }
  size_t size() const { return size_; }
  Pod* begin() const { return data_; }
  Pod* end() const { return data_ + size_; }
  Pod const* cbegin() const { return begin(); }
  Pod const* cend() const { return end(); }
  Buffer& operator+=(size_t n)
  {
    n = std::min(n, size_);
    data_ += n;
    size_ -= n;
    return *this;
  }

private:
  Pod* data_ = nullptr;
  size_t size_ = 0;
};

template <PodType Pod> Buffer<Pod> operator+(Buffer<Pod> const& b, size_t n)
{
  auto ret = b;
  ret += n;
  return ret;
}

template <PodType Pod> Buffer<Pod> operator+(size_t n, Buffer<Pod> const& b) { return b + n; }

template <PodType Pod> using ConstBuffer = Buffer<Pod const>;
template <PodType Pod, std::enable_if_t<!std::is_const_v<Pod>, int> = 0>
using MutableBuffer = Buffer<Pod>;

}  // namespace pichi

#endif  // PICHI_COMMON_BUFFER_HPP
