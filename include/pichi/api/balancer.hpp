#ifndef PICHI_API_BALANCER_HPP
#define PICHI_API_BALANCER_HPP

#include <iterator>
#include <memory>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <vector>

namespace pichi::api {

namespace detail {

template <typename Offset> struct BalanceSelector {
  virtual Offset select() = 0;
  virtual void release(Offset) = 0;
  virtual ~BalanceSelector() = default;
};

}  // namespace detail

class Balancer {
private:
  using Endpoints = std::vector<Endpoint>;
  using EndpointIt = typename Endpoints::const_iterator;
  using Offset = std::iterator_traits<EndpointIt>::difference_type;
  using SelectorPtr = std::unique_ptr<detail::BalanceSelector<Offset>>;

  static SelectorPtr makeSelector(BalanceType, EndpointIt, EndpointIt);

public:
  using Iterator = EndpointIt;

  template <typename InputIt>
  Balancer(BalanceType type, InputIt first, InputIt last)
    : endpoints_{first, last},
      selector_{makeSelector(type, std::cbegin(endpoints_), std::cend(endpoints_))}
  {
  }

  Iterator select();
  void release(Iterator);

private:
  Endpoints endpoints_;
  SelectorPtr selector_;
};

}  // namespace pichi::api

#endif  // PICHI_API_BALANCER_HPP
