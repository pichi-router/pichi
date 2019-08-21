#ifndef PICHI_API_BALANCER_HPP
#define PICHI_API_BALANCER_HPP

#include <memory>
#include <pichi/api/vos.hpp>

namespace pichi::api {

template <typename ForwardIt> struct Balancer {
  using Iterator = ForwardIt;
  virtual ForwardIt select() = 0;
  virtual void release(ForwardIt) = 0;
  virtual ~Balancer() = default;
};

template <typename ForwardIt>
std::unique_ptr<Balancer<ForwardIt>> makeBalancer(BalanceType, ForwardIt, ForwardIt);

} // namespace pichi::api

#endif // PICHI_API_BALANCER_HPP
