#define BOOST_TEST_MODULE pichi balancer test

#include "utils.hpp"
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <pichi/api/balancer.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace pichi;
using namespace pichi::api;
namespace mpl = boost::mpl;

using RawPointer = int*;
using VectorIter = vector<int>::const_iterator;

using Iterators = mpl::list<RawPointer, VectorIter>;

static size_t const N = 100_sz;

template <typename Iterator> struct Traits {
};

template <> struct Traits<RawPointer> {
  using Iterator = RawPointer;
  static Iterator const FIRST;
  static Iterator const LAST;
};

template <> struct Traits<VectorIter> {
  using Iterator = VectorIter;
  static vector<int> const DATA;
  static Iterator const FIRST;
  static Iterator const LAST;
};

Traits<int*>::Iterator const Traits<int*>::FIRST = nullptr;
Traits<int*>::Iterator const Traits<int*>::LAST = FIRST + N;

vector<int> const Traits<VectorIter>::DATA(N, 0);
Traits<VectorIter>::Iterator const Traits<VectorIter>::FIRST = begin(DATA);
Traits<VectorIter>::Iterator const Traits<VectorIter>::LAST = end(DATA);

BOOST_AUTO_TEST_SUITE(BALANCER)

BOOST_AUTO_TEST_CASE_TEMPLATE(select_Empty, Iterator, Iterators)
{
  for (auto type : {BalanceType::RANDOM, BalanceType::ROUND_ROBIN, BalanceType::LEAST_CONN}) {
    BOOST_CHECK_EXCEPTION(makeBalancer(type, Traits<Iterator>::FIRST, Traits<Iterator>::FIRST),
                          Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(select_Wrong_Order, Iterator, Iterators)
{
  for (auto type : {BalanceType::RANDOM, BalanceType::ROUND_ROBIN, BalanceType::LEAST_CONN}) {
    BOOST_CHECK_EXCEPTION(makeBalancer(type, Traits<Iterator>::LAST, Traits<Iterator>::FIRST),
                          Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(RANDOM_select_Probability, Iterator, Iterators)
{
  using Difference = typename iterator_traits<Iterator>::difference_type;
  auto const K = 1000;
  auto data = unordered_map<Difference, int>{};
  auto balancer =
      makeBalancer(BalanceType::RANDOM, Traits<Iterator>::FIRST, Traits<Iterator>::LAST);
  for (auto i = 0_sz; i < N * K; ++i) ++data[distance(Traits<Iterator>::FIRST, balancer->select())];

  BOOST_CHECK_EQUAL(N, data.size());
  for_each(cbegin(data), cend(data), [=, delta = sqrt(N * K) / 2.0](auto&& item) {
    BOOST_CHECK_LE(abs(item.second - K), delta);
  });
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ROUND_ROBIN_select, Iterator, Iterators)
{
  auto balancer =
      makeBalancer(BalanceType::ROUND_ROBIN, Traits<Iterator>::FIRST, Traits<Iterator>::LAST);
  for (auto it = Traits<Iterator>::FIRST; it != Traits<Iterator>::LAST; ++it)
    BOOST_CHECK(balancer->select() == it);
  for (auto it = Traits<Iterator>::FIRST; it != Traits<Iterator>::LAST; ++it)
    BOOST_CHECK(balancer->select() == it);
  for (auto it = Traits<Iterator>::FIRST; it != Traits<Iterator>::LAST; ++it)
    BOOST_CHECK(balancer->select() == it);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(LEAST_CONN_select, Iterator, Iterators)
{
  using Difference = typename iterator_traits<Iterator>::difference_type;
  auto container = unordered_set<Difference>{};
  auto balancer =
      makeBalancer(BalanceType::LEAST_CONN, Traits<Iterator>::FIRST, Traits<Iterator>::LAST);
  for (auto i = 0_sz; i < N; ++i) {
    generate_n(inserter(container, end(container)), N, [&balancer]() {
      auto it = balancer->select();
      return distance(Traits<Iterator>::FIRST, it);
    });
    BOOST_CHECK_EQUAL(N, container.size());
    container.clear();
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(LEAST_CONN_release_No_Connection_Iterator, Iterator, Iterators)
{
  auto balancer =
      makeBalancer(BalanceType::LEAST_CONN, Traits<Iterator>::FIRST, Traits<Iterator>::LAST);
  for (auto it = Traits<Iterator>::FIRST; it != Traits<Iterator>::LAST; ++it)
    BOOST_CHECK_EXCEPTION(balancer->release(it), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(LEAST_CONN_release, Iterator, Iterators)
{
  auto balancer =
      makeBalancer(BalanceType::LEAST_CONN, Traits<Iterator>::FIRST, Traits<Iterator>::LAST);
  for (auto i = 0_sz; i < N; ++i) balancer->select();

  for (auto i = 0_sz; i < N; ++i) {
    balancer->release(Traits<Iterator>::FIRST);
    BOOST_CHECK(Traits<Iterator>::FIRST == balancer->select());
  }
}

BOOST_AUTO_TEST_SUITE_END()
