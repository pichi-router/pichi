#ifndef PICHI_API_BALANCER_HPP
#define PICHI_API_BALANCER_HPP

#include <cassert>
#include <iterator>
#include <map>
#include <memory>
#include <pichi/api/vos.hpp>
#include <pichi/asserts.hpp>
#include <pichi/common.hpp>
#include <random>
#include <unordered_map>
#include <unordered_set>

namespace pichi::api {

template <typename ForwardIt> struct Balancer {
  using Iterator = ForwardIt;
  virtual ForwardIt select() = 0;
  virtual void release(ForwardIt) = 0;
  virtual ~Balancer() = default;
};

namespace detail {

template <typename ForwardIt> class RandomBalancer : public Balancer<ForwardIt> {
private:
  using Difference = typename std::iterator_traits<ForwardIt>::difference_type;

public:
  explicit RandomBalancer(ForwardIt first, ForwardIt last)
    : first_{first}, g_{std::random_device{}()}, rand_{0, std::distance(first, last) - 1}
  {
    assertTrue(rand_.b() >= rand_.a());
  }

  RandomBalancer(RandomBalancer const&) = delete;
  RandomBalancer(RandomBalancer&&) = delete;
  RandomBalancer& operator=(RandomBalancer const&) = delete;
  RandomBalancer& operator=(RandomBalancer&&) = delete;
  ~RandomBalancer() override = default;

  ForwardIt select() override
  {
    auto ret = first_;
    std::advance(ret, rand_(g_));
    return ret;
  }

  void release(ForwardIt) override {}

private:
  ForwardIt first_;
  std::mt19937_64 g_;
  std::uniform_int_distribution<Difference> rand_;
};

template <typename ForwardIt> class RoundRobinBalancer : public Balancer<ForwardIt> {
public:
  explicit RoundRobinBalancer(ForwardIt first, ForwardIt last)
    : first_{first}, last_{last}, current_{first}
  {
    assertTrue(std::distance(first, last) > 0);
  }

  RoundRobinBalancer(RoundRobinBalancer const&) = delete;
  RoundRobinBalancer(RoundRobinBalancer&&) = delete;
  RoundRobinBalancer& operator=(RoundRobinBalancer const&) = delete;
  RoundRobinBalancer& operator=(RoundRobinBalancer&&) = delete;
  ~RoundRobinBalancer() override = default;

  ForwardIt select() override
  {
    auto ret = current_++;
    if (current_ == last_) current_ = first_;
    return ret;
  }

  void release(ForwardIt) override {}

private:
  ForwardIt first_;
  ForwardIt last_;
  ForwardIt current_;
};

template <typename ForwardIt> class LeastConnBalancer : public Balancer<ForwardIt> {
private:
  using Difference = typename std::iterator_traits<ForwardIt>::difference_type;
  using Container = std::map<size_t, std::unordered_set<Difference>>;
  using Item = typename Container::iterator;
  using Index = std::unordered_map<Difference, typename Container::iterator>;

  static auto initialize(ForwardIt first, ForwardIt last)
  {
    assertTrue(std::distance(first, last) > 0);
    auto ret = std::unordered_set<Difference>{};
    for (auto it = first; it != last; ++it) ret.insert(std::distance(first, it));
    return ret;
  }

  static auto buildIndex(Container& c)
  {
    auto index = Index{};
    for (auto it = std::begin(c); it != std::end(c); ++it) {
      std::transform(std::cbegin(it->second), std::cend(it->second),
                     std::inserter(index, std::end(index)),
                     [it](Difference delta) { return std::make_pair(delta, it); });
    }
    return index;
  }

  auto remove(Item item, typename std::unordered_set<Difference>::iterator it)
  {
    auto&& [conn, its] = *item;
    assertFalse(it == end(its));
    auto ret = *it;
    its.erase(it);
    if (its.empty()) c_.erase(item);
    return ret;
  }

  auto remove(Item item) { return remove(item, std::begin(item->second)); }

  auto remove(Item item, Difference delta) { return remove(item, item->second.find(delta)); }

  auto insert(size_t conn, Difference delta)
  {
    assertTrue(c_[conn].insert(delta).second);
    return c_.find(conn);
  }

public:
  explicit LeastConnBalancer(ForwardIt first, ForwardIt last)
    : first_{first}, c_{{0_sz, initialize(first, last)}}, index_{buildIndex(c_)}
  {
  }

  LeastConnBalancer(LeastConnBalancer const&) = delete;
  LeastConnBalancer(LeastConnBalancer&&) = delete;
  LeastConnBalancer& operator=(LeastConnBalancer const&) = delete;
  LeastConnBalancer& operator=(LeastConnBalancer&&) = delete;
  ~LeastConnBalancer() = default;

  ForwardIt select() override
  {
    auto conn = std::begin(c_)->first;
    auto delta = remove(std::begin(c_));
    index_[delta] = insert(++conn, delta);
    auto ret = first_;
    std::advance(ret, delta);
    return ret;
  }

  void release(ForwardIt it) override
  {
    auto delta = std::distance(first_, it);
    assertFalse(index_[delta]->first == 0);
    remove(index_[delta], delta);
    index_[delta] = insert(index_[delta]->first - 1, delta);
  }

private:
  ForwardIt first_;
  Container c_;
  Index index_;
};

} // namespace detail

template <typename ForwardIt>
std::unique_ptr<Balancer<ForwardIt>> makeBalancer(BalanceType type, ForwardIt first, ForwardIt last)
{
  switch (type) {
  case BalanceType::RANDOM:
    return std::make_unique<detail::RandomBalancer<ForwardIt>>(first, last);
  case BalanceType::ROUND_ROBIN:
    return std::make_unique<detail::RoundRobinBalancer<ForwardIt>>(first, last);
  case BalanceType::LEAST_CONN:
    return std::make_unique<detail::LeastConnBalancer<ForwardIt>>(first, last);
  default:
    fail();
  }
}

} // namespace pichi::api

#endif // PICHI_API_BALANCER_HPP
