#include <pichi/common/config.hpp>
// Include config.hpp first
#include <map>
#include <pichi/api/balancer.hpp>
#include <pichi/api/ingress_holder.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <random>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace pichi::api {

namespace detail {

template <typename Offset> class Random : public BalanceSelector<Offset> {
public:
  Random(Offset capacity) : g_{random_device{}()}, rand_{0, capacity - 1} {}

  Offset select() override { return rand_(g_); }

  void release(Offset) override {}

private:
  mt19937_64 g_;
  uniform_int_distribution<Offset> rand_;
};

template <typename Offset> class RoundRobin : public BalanceSelector<Offset> {
public:
  RoundRobin(Offset capacity) : current_{0}, capacity_{capacity} {}

  Offset select() override
  {
    auto ret = current_++;
    if (current_ == capacity_) current_ = 0;
    return ret;
  }

  void release(Offset) override {}

private:
  Offset current_;
  Offset capacity_;
};

template <typename Offset> class LeastConn : public BalanceSelector<Offset> {
private:
  using Offsets = unordered_set<Offset>;
  using SortedDb = map<size_t, Offsets>;
  using DbIt = typename SortedDb::iterator;
  using OffsetIt = typename Offsets::iterator;
  using ReversedIndex = unordered_map<Offset, DbIt>;

  static auto initDb(Offset capacity)
  {
    auto ret = SortedDb{{0_sz, Offsets{}}};
    auto& offsets = ret[0_sz];
    for (Offset i = 0; i < capacity; ++i) offsets.emplace(i);
    return ret;
  }

  static auto initIndex(DbIt first, DbIt last)
  {
    auto index = ReversedIndex{};
    for (auto it = first; it != last; ++it) {
      transform(cbegin(it->second), cend(it->second), inserter(index, end(index)),
                [it](auto offset) { return make_pair(offset, it); });
    }
    return index;
  }

  auto popOffset(DbIt dbIt, OffsetIt offsetIt)
  {
    auto& offsets = dbIt->second;
    auto ret = *offsetIt;
    offsets.erase(offsetIt);
    if (offsets.empty()) db_.erase(dbIt);
    return ret;
  }

  auto insertItem(size_t conn, Offset offset)
  {
    auto [dbIt, _] = db_.emplace(conn, Offsets{});
    dbIt->second.emplace(offset);
    return dbIt;
  }

public:
  explicit LeastConn(Offset capacity)
    : db_{initDb(capacity)}, index_{initIndex(begin(db_), end(db_))}
  {
  }

  Offset select() override
  {
    assertFalse(db_.empty());
    auto least = begin(db_);
    auto conn = least->first;
    auto offset = popOffset(least, begin(least->second));
    index_[offset] = insertItem(++conn, offset);
    return offset;
  }

  void release(Offset offset) override
  {
    auto idxIt = index_.find(offset);
    assertFalse(idxIt == end(index_));
    auto dbIt = idxIt->second;
    auto conn = dbIt->first;
    assertFalse(conn == 0);
    popOffset(dbIt, dbIt->second.find(offset));
    index_[offset] = insertItem(--conn, offset);
  }

private:
  SortedDb db_;
  ReversedIndex index_;
};

}  // namespace detail

Balancer::SelectorPtr Balancer::makeSelector(BalanceType type, EndpointIt first, EndpointIt last)
{
  auto capacity = distance(first, last);
  assertTrue(capacity > 0);
  switch (type) {
  case BalanceType::RANDOM:
    return make_unique<detail::Random<Offset>>(capacity);
  case BalanceType::ROUND_ROBIN:
    return make_unique<detail::RoundRobin<Offset>>(capacity);
  case BalanceType::LEAST_CONN:
    return make_unique<detail::LeastConn<Offset>>(capacity);
  default:
    fail();
  }
}

Balancer::Iterator Balancer::select()
{
  auto ret = cbegin(endpoints_);
  advance(ret, selector_->select());
  return ret;
}

void Balancer::release(Iterator it)
{
  auto offset = distance(cbegin(endpoints_), it);
  assertTrue(offset >= 0);
  selector_->release(offset);
}

}  // namespace pichi::api
