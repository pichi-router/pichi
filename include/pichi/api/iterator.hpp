#ifndef PICHI_API_ITERATOR_HPP
#define PICHI_API_ITERATOR_HPP

#include <functional>
#include <iterator>
#include <optional>
#include <string_view>
#include <type_traits>

namespace pichi {
namespace api {

template <typename T> struct IsPair : public std::false_type {
};

template <typename T1, typename T2> struct IsPair<std::pair<T1, T2>> : public std::true_type {
};

template <typename T> inline constexpr bool IsPairV = IsPair<T>::value;

template <typename Delegate, typename ValueType> class Iterator {
private:
  static_assert(std::is_base_of_v<std::forward_iterator_tag,
                                  typename std::iterator_traits<Delegate>::iterator_category>);
  static_assert(IsPairV<ValueType>);
  static_assert(std::is_copy_constructible_v<ValueType>);
  using Holder = std::optional<ValueType>;
  using Generator = std::function<ValueType(Delegate)>;

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = ValueType;
  using difference_type = typename std::iterator_traits<Delegate>::difference_type;
  using reference = value_type const&;
  using pointer = value_type const*;

public:
  Iterator(Delegate delegate, Delegate last, Generator g) : delegate_{delegate}, last_{last}, g_{g}
  {
    if (delegate_ != last_) holder_.emplace(g_(delegate_));
  }

  reference operator*() const { return *holder_; }

  pointer operator->() const { return std::addressof(*holder_); }

  Iterator& operator++()
  {
    if (++delegate_ != last_)
      holder_.emplace(g_(delegate_));
    else
      holder_.reset();
    return *this;
  }

  Iterator operator++(int)
  {
    auto ret = *this;
    ++(*this);
    return ret;
  }

  friend inline bool operator==(Iterator const& lhs, Iterator const& rhs)
  {
    return lhs.delegate_ == rhs.delegate_;
  }

  friend inline bool operator!=(Iterator const& lhs, Iterator const& rhs)
  {
    return lhs.delegate_ != rhs.delegate_;
  }

private:
  Delegate delegate_;
  Delegate last_;
  Generator g_;
  Holder holder_ = {};
};

} // namespace api
} // namespace pichi

#endif // PICHI_API_ITERATOR_HPP
