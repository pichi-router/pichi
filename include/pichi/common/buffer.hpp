#ifndef PICHI_COMMON_BUFFER_HPP
#define PICHI_COMMON_BUFFER_HPP

#include <cassert>
#include <stddef.h>
#include <type_traits>

namespace pichi {

template <typename PodType> class Buffer;

template <typename PodType> Buffer<PodType> operator+(Buffer<PodType> const&, size_t);
template <typename PodType> Buffer<PodType> operator+(size_t, Buffer<PodType> const&);

// TODO avoid lots of constructors if conditional explicit is supported
template <typename PodType> class Buffer {
private:
  static_assert(std::is_pod_v<PodType>, "value type of Buffer must be POD");

  enum class ConversionType { IMPLICIT, STATIC, EXPLICIT, UNKNOWN };

  template <typename From, typename To> static constexpr ConversionType detect()
  {
    if constexpr (std::is_const_v<From> && !std::is_const_v<To>)
      return ConversionType::UNKNOWN;
    else if constexpr (std::is_convertible_v<From*, To*>)
      return ConversionType::IMPLICIT;
    else if constexpr (std::is_same_v<std::decay_t<From>, void>)
      return ConversionType::STATIC;
    else if constexpr (sizeof(From) == sizeof(To))
      return ConversionType::EXPLICIT;
    else
      return ConversionType::UNKNOWN;
  }

  template <typename C, typename P = decltype(std::declval<C>().data()),
            typename S = decltype(std::declval<C>().size())>
  struct Helper {
    static_assert(std::is_convertible_v<S, size_t>);
    static_assert(std::is_pointer_v<P>);
    using ValueType = std::remove_pointer_t<P>;
  };

public:
  Buffer() = default;

  Buffer(PodType* data, size_t size) : data_{data}, size_{size} {}

  template <size_t N> Buffer(PodType (&data)[N]) : Buffer{std::decay_t<decltype(data)>(&data), N} {}

  template <typename Container,
            ConversionType t = detect<typename Helper<Container>::ValueType, PodType>()>
  Buffer(Container&& c, size_t size, std::enable_if_t<t == ConversionType::IMPLICIT>* = nullptr)
    : Buffer{c.data(), size}
  {
    assert(size <= c.size());
  }

  template <typename Container,
            ConversionType t = detect<typename Helper<Container>::ValueType, PodType>()>
  Buffer(Container&& c, std::enable_if_t<t == ConversionType::IMPLICIT>* = nullptr)
    : Buffer{std::forward<Container>(c), c.size()}
  {
  }

  template <typename Container,
            ConversionType t = detect<typename Helper<Container>::ValueType, PodType>()>
  Buffer(Container&& c, size_t size, std::enable_if_t<t == ConversionType::STATIC>* = nullptr)
    : Buffer{static_cast<PodType*>(c.data()), size}
  {
    assert(size <= c.size());
  }

  template <typename Container,
            ConversionType t = detect<typename Helper<Container>::ValueType, PodType>()>
  Buffer(Container&& c, std::enable_if_t<t == ConversionType::STATIC>* = nullptr)
    : Buffer{std::forward<Container>(c), c.size()}
  {
  }

  template <typename Container,
            ConversionType t = detect<typename Helper<Container>::ValueType, PodType>()>
  explicit Buffer(Container&& c, size_t size,
                  std::enable_if_t<t == ConversionType::EXPLICIT>* = nullptr)
    : Buffer{reinterpret_cast<PodType*>(c.data()), size}
  {
    assert(size <= c.size());
  }

  template <typename Container,
            ConversionType t = detect<typename Helper<Container>::ValueType, PodType>()>
  explicit Buffer(Container&& c, std::enable_if_t<t == ConversionType::EXPLICIT>* = nullptr)
    : Buffer{std::forward<Container>(c), c.size()}
  {
  }

public:
  PodType* data() const { return data_; }
  size_t size() const { return size_; }
  PodType* begin() const { return data_; }
  PodType* end() const { return data_ + size_; }
  PodType const* cbegin() const { return begin(); }
  PodType const* cend() const { return end(); }
  Buffer& operator+=(size_t n)
  {
    n = std::min(n, size_);
    data_ += n;
    size_ -= n;
    return *this;
  }

private:
  PodType* data_ = nullptr;
  size_t size_ = 0;
};

template <typename PodType> Buffer<PodType> operator+(Buffer<PodType> const& b, size_t n)
{
  auto ret = b;
  ret += n;
  return ret;
}

template <typename PodType> Buffer<PodType> operator+(size_t n, Buffer<PodType> const& b)
{
  return b + n;
}

template <typename PodType> using ConstBuffer = Buffer<PodType const>;
template <typename PodType, std::enable_if_t<!std::is_const_v<PodType>, int> = 0>
using MutableBuffer = Buffer<PodType>;

}  // namespace pichi

#endif  // PICHI_COMMON_BUFFER_HPP
