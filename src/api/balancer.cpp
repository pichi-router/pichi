#include <cassert>
#include <iterator>
#include <map>
#include <pichi/api/balancer.hpp>
#include <pichi/asserts.hpp>
#include <pichi/common.hpp>
#include <pichi/config.hpp>
#include <random>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace pichi::api {

template <typename ForwardIt> class RandomBalancer : public Balancer<ForwardIt> {
private:
  using Difference = typename iterator_traits<ForwardIt>::difference_type;

public:
  explicit RandomBalancer(ForwardIt, ForwardIt);

  RandomBalancer(RandomBalancer const&) = delete;
  RandomBalancer(RandomBalancer&&) = delete;
  RandomBalancer& operator=(RandomBalancer const&) = delete;
  RandomBalancer& operator=(RandomBalancer&&) = delete;
  ~RandomBalancer() override = default;

  ForwardIt select() override;
  void release(ForwardIt) override;

private:
  ForwardIt first_;
  mt19937_64 g_;
  uniform_int_distribution<Difference> rand_;
};

template <typename ForwardIt> class RoundRobinBalancer : public Balancer<ForwardIt> {
public:
  explicit RoundRobinBalancer(ForwardIt, ForwardIt);

  RoundRobinBalancer(RoundRobinBalancer const&) = delete;
  RoundRobinBalancer(RoundRobinBalancer&&) = delete;
  RoundRobinBalancer& operator=(RoundRobinBalancer const&) = delete;
  RoundRobinBalancer& operator=(RoundRobinBalancer&&) = delete;
  ~RoundRobinBalancer() override = default;

  ForwardIt select() override;

  void release(ForwardIt) override;

private:
  ForwardIt first_;
  ForwardIt last_;
  ForwardIt current_;
};

template <typename ForwardIt> class LeastConnBalancer : public Balancer<ForwardIt> {
private:
  using Difference = typename iterator_traits<ForwardIt>::difference_type;
  using Container = map<size_t, unordered_set<Difference>>;
  using Item = typename Container::iterator;
  using Index = unordered_map<Difference, typename Container::iterator>;

  static auto initialize(ForwardIt first, ForwardIt last);

  static auto buildIndex(Container&);

  auto remove(Item item, typename unordered_set<Difference>::iterator);

  auto remove(Item);

  auto remove(Item, Difference);

  auto insert(size_t, Difference);

public:
  explicit LeastConnBalancer(ForwardIt, ForwardIt);

  LeastConnBalancer(LeastConnBalancer const&) = delete;
  LeastConnBalancer(LeastConnBalancer&&) = delete;
  LeastConnBalancer& operator=(LeastConnBalancer const&) = delete;
  LeastConnBalancer& operator=(LeastConnBalancer&&) = delete;
  ~LeastConnBalancer() = default;

  ForwardIt select() override;

  void release(ForwardIt) override;

private:
  ForwardIt first_;
  Container c_;
  Index index_;
};

template <typename ForwardIt>
RandomBalancer<ForwardIt>::RandomBalancer(ForwardIt first, ForwardIt last)
  : first_{first}, g_{random_device{}()}, rand_{0, distance(first, last) - 1}
{
  assertTrue(rand_.b() >= rand_.a());
}

template <typename ForwardIt> ForwardIt RandomBalancer<ForwardIt>::select()
{
  auto ret = first_;
  advance(ret, rand_(g_));
  return ret;
}

template <typename ForwardIt> void RandomBalancer<ForwardIt>::release(ForwardIt) {}

template <typename ForwardIt>
RoundRobinBalancer<ForwardIt>::RoundRobinBalancer(ForwardIt first, ForwardIt last)
  : first_{first}, last_{last}, current_{first}
{
  assertTrue(distance(first, last) > 0);
}

template <typename ForwardIt> ForwardIt RoundRobinBalancer<ForwardIt>::select()
{
  auto ret = current_++;
  if (current_ == last_) current_ = first_;
  return ret;
}

template <typename ForwardIt> void RoundRobinBalancer<ForwardIt>::release(ForwardIt) {}

template <typename ForwardIt>
auto LeastConnBalancer<ForwardIt>::initialize(ForwardIt first, ForwardIt last)
{
  assertTrue(distance(first, last) > 0);
  auto ret = unordered_set<Difference>{};
  for (auto it = first; it != last; ++it) ret.insert(distance(first, it));
  return ret;
}

template <typename ForwardIt> auto LeastConnBalancer<ForwardIt>::buildIndex(Container& c)
{
  auto index = Index{};
  for (auto it = begin(c); it != end(c); ++it) {
    transform(cbegin(it->second), cend(it->second), inserter(index, end(index)),
              [it](Difference delta) { return make_pair(delta, it); });
  }
  return index;
}

template <typename ForwardIt>
auto LeastConnBalancer<ForwardIt>::remove(Item item,
                                          typename unordered_set<Difference>::iterator it)
{
  auto&& [conn, its] = *item;
  assertFalse(it == end(its));
  auto ret = *it;
  its.erase(it);
  if (its.empty()) c_.erase(item);
  return ret;
}

template <typename ForwardIt> auto LeastConnBalancer<ForwardIt>::remove(Item item)
{
  return remove(item, begin(item->second));
}

template <typename ForwardIt> auto LeastConnBalancer<ForwardIt>::remove(Item item, Difference delta)
{
  return remove(item, item->second.find(delta));
}

template <typename ForwardIt>
auto LeastConnBalancer<ForwardIt>::insert(size_t conn, Difference delta)
{
  assertTrue(c_[conn].insert(delta).second);
  return c_.find(conn);
}

template <typename ForwardIt>
LeastConnBalancer<ForwardIt>::LeastConnBalancer(ForwardIt first, ForwardIt last)
  : first_{first}, c_{{0_sz, initialize(first, last)}}, index_{buildIndex(c_)}
{
}

template <typename ForwardIt> ForwardIt LeastConnBalancer<ForwardIt>::select()
{
  auto conn = begin(c_)->first;
  auto delta = remove(begin(c_));
  index_[delta] = insert(++conn, delta);
  auto ret = first_;
  advance(ret, delta);
  return ret;
}

template <typename ForwardIt> void LeastConnBalancer<ForwardIt>::release(ForwardIt it)
{
  auto delta = distance(first_, it);
  assertFalse(index_[delta]->first == 0);
  remove(index_[delta], delta);
  index_[delta] = insert(index_[delta]->first - 1, delta);
}

template <typename ForwardIt>
unique_ptr<Balancer<ForwardIt>> makeBalancer(BalanceType type, ForwardIt first, ForwardIt last)
{
  switch (type) {
  case BalanceType::RANDOM:
    return make_unique<RandomBalancer<ForwardIt>>(first, last);
  case BalanceType::ROUND_ROBIN:
    return make_unique<RoundRobinBalancer<ForwardIt>>(first, last);
  case BalanceType::LEAST_CONN:
    return make_unique<LeastConnBalancer<ForwardIt>>(first, last);
  default:
    fail();
  }
}

using IteratorForIngress = decltype(IngressVO::destinations_)::const_iterator;
template unique_ptr<Balancer<IteratorForIngress>> makeBalancer<>(BalanceType, IteratorForIngress,
                                                                 IteratorForIngress);

#ifdef BUILD_TEST
template unique_ptr<Balancer<int*>> makeBalancer<>(BalanceType, int*, int*);
using VectorIter = vector<int>::const_iterator;
template unique_ptr<Balancer<VectorIter>> makeBalancer<>(BalanceType, VectorIter, VectorIter);
#endif // BUILD_TEST

} // namespace pichi::api
