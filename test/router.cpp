#define BOOST_TEST_MODULE pichi router test

#include "utils.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/api/router.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
using ResolvedResults = ip::basic_resolver_results<ip::tcp>;

namespace pichi::unit_test {

using namespace api;
using vo::Route;

static decltype(auto) fn = "geo.mmdb";

static ResolvedResults createRR(string_view str = ""sv)
{
  return str.empty()
             ? ResolvedResults{}
             : ResolvedResults::create(ip::tcp::endpoint{ip::make_address(str), 443}, ph, ph);
}

BOOST_AUTO_TEST_SUITE(ROUTER_TEST)

BOOST_AUTO_TEST_CASE(matchDomain_Empty_Domains)
{
  BOOST_CHECK_EXCEPTION(matchDomain("example.com", ""), SystemError,
                        verifyException<PichiError::SEMANTIC_ERROR>);
  BOOST_CHECK_EXCEPTION(matchDomain("", "example.com"), SystemError,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(matchDomain_Domains_Start_With_Dot)
{
  BOOST_CHECK_EXCEPTION(matchDomain(".", "com"), SystemError, verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(matchDomain(".com", "com"), SystemError, verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(matchDomain("example.com", "."), SystemError,
                        verifyException<PichiError::SEMANTIC_ERROR>);
  BOOST_CHECK(matchDomain("example.com", ".com"));
}

BOOST_AUTO_TEST_CASE(matchDomain_Matched)
{
  BOOST_CHECK(matchDomain("foo.bar.example.com", "bar.example.com"));
  BOOST_CHECK(matchDomain("foo.bar.example.com", "example.com"));
  BOOST_CHECK(matchDomain("foo.bar.example.com", "com"));
}

BOOST_AUTO_TEST_CASE(matchDomain_Same_End)
{
  BOOST_CHECK(!matchDomain("foobar.example.com", "bar.example.com"));
  BOOST_CHECK(!matchDomain("foobarexample.com", "example.com"));
  BOOST_CHECK(!matchDomain("example.com", "m"));
}

BOOST_AUTO_TEST_CASE(matchDomain_Containing_Not_Matched)
{
  BOOST_CHECK(!matchDomain("example.com", "example"));
  BOOST_CHECK(!matchDomain("foo.example.com", "example"));
  BOOST_CHECK(!matchDomain("example.com", "e.c"));
}

BOOST_AUTO_TEST_CASE(matchDomain_Same)
{
  BOOST_CHECK(matchDomain("example.com", "example.com"));
  BOOST_CHECK(matchDomain("foo.example.com", "foo.example.com"));
}

BOOST_AUTO_TEST_CASE(matchDomain_Case_Insensitive_Matching)
{
  BOOST_CHECK(matchDomain("EXAMPLE.COM", "com"));
  BOOST_CHECK(matchDomain("example.com", "COM"));
  BOOST_CHECK(matchDomain("Example.com", "Com"));
  BOOST_CHECK(matchDomain("EXAMPLE.COM", "example.com"));
  BOOST_CHECK(matchDomain("example.com", "EXAMPLE.COM"));
  BOOST_CHECK(matchDomain("Example.com", "example.Com"));
}

BOOST_AUTO_TEST_CASE(Router_Empty_Rules)
{
  auto router = Router{fn};
  BOOST_CHECK(begin(router) == end(router));

  router.update(ph, {});
  BOOST_CHECK(begin(router) != end(router));

  router.erase(ph);
  BOOST_CHECK(begin(router) == end(router));
}

BOOST_AUTO_TEST_CASE(Router_isUsed)
{
  auto router = Router{fn};
  BOOST_CHECK(!router.isUsed(ph));

  router.update(ph, {});
  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});
  BOOST_CHECK(router.isUsed(ph));

  router.setRoute({ph});
  BOOST_CHECK(router.isUsed(ph));
}

BOOST_AUTO_TEST_CASE(Router_Erase_Not_Existing)
{
  auto router = Router{fn};
  BOOST_CHECK(begin(router) == end(router));

  router.erase(ph);
  BOOST_CHECK(begin(router) == end(router));
}

BOOST_AUTO_TEST_CASE(Router_Erase_Rule_Used_By_Route)
{
  auto router = Router{fn};
  router.update(ph, {});
  router.setRoute({{}, {make_pair(vector<string>{ph}, "")}});

  BOOST_CHECK_EXCEPTION(router.erase(ph), SystemError, verifyException<PichiError::RES_IN_USE>);

  router.setRoute({});
  router.erase(ph);
  BOOST_CHECK(begin(router) == end(router));
}

BOOST_AUTO_TEST_CASE(Router_Iteration)
{
  static auto const MAX = 10;
  auto router = Router{fn};
  BOOST_CHECK(begin(router) == end(router));

  for (auto i = 0; i < MAX; ++i) router.update(to_string(i), {});

  BOOST_CHECK(begin(router) != end(router));
  BOOST_CHECK(distance(begin(router), end(router)) == MAX);

  for (auto i = 0; i < MAX; ++i) {
    auto s = to_string(i);
    // TODO use find_if when VC++ doesn't require Iterator's copy assignment operator
    auto it = begin(router);
    while (it != end(router) && it->first != s) ++it;
    BOOST_CHECK(it != end(router));
  }
}

BOOST_AUTO_TEST_CASE(Router_Set_Not_Existing_Rule)
{
  auto verifyDefault = [](auto&& rvo) {
    BOOST_CHECK(rvo.default_.has_value());
    BOOST_CHECK(*rvo.default_ == vo::type::DIRECT);
    BOOST_CHECK(rvo.rules_.empty());
  };

  auto router = Router{fn};
  verifyDefault(router.getRoute());

  BOOST_CHECK_EXCEPTION(router.setRoute({{}, {make_pair(vector<string>{ph}, "")}}), SystemError,
                        verifyException<PichiError::SEMANTIC_ERROR>);
  verifyDefault(router.getRoute());
}

BOOST_AUTO_TEST_CASE(Router_Set_Default_Route)
{
  auto router = Router{fn};
  auto vo = router.getRoute();
  BOOST_CHECK(vo.default_.has_value());
  BOOST_CHECK(*vo.default_ == vo::type::DIRECT);
  BOOST_CHECK(vo.rules_.empty());

  router.setRoute({ph});
  vo = router.getRoute();
  BOOST_CHECK(vo.default_.has_value());
  BOOST_CHECK(*vo.default_ == ph);
  BOOST_CHECK(vo.rules_.empty());
}

BOOST_AUTO_TEST_CASE(Router_setRoute_With_Rules)
{
  static auto const MAX = 10;
  auto verifyRules = [](auto&& expect, auto&& fact) {
    BOOST_CHECK(equal(cbegin(expect), cend(expect), cbegin(fact), cend(fact)));
  };

  auto router = Router{fn};

  for (auto i = 0; i < MAX; ++i) router.update(to_string(i), {});

  auto seq = Route{};
  for (auto i = 0; i < MAX; ++i)
    seq.rules_.push_back(make_pair(vector<string>{to_string(i)}, to_string(i)));
  router.setRoute(seq);
  verifyRules(seq.rules_, router.getRoute().rules_);

  auto rev = Route{};
  for (auto i = MAX - 1; i >= 0; --i)
    seq.rules_.push_back(make_pair(vector<string>{to_string(i)}, to_string(i)));
  router.setRoute(rev);
  verifyRules(rev.rules_, router.getRoute().rules_);
}

BOOST_AUTO_TEST_CASE(Router_setRoute_Without_Default)
{
  auto route = Route{ph, {make_pair(vector<string>{ph}, ph)}};
  auto router = Router{fn};
  router.update(ph, {});
  router.setRoute(route);

  router.setRoute({});
  auto emptyRules = router.getRoute();
  BOOST_CHECK(emptyRules.default_.has_value());
  BOOST_CHECK_EQUAL(ph, *emptyRules.default_);
  BOOST_CHECK(emptyRules.rules_.empty());

  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});
  auto withRules = router.getRoute();
  BOOST_CHECK(withRules.default_.has_value());
  BOOST_CHECK_EQUAL(ph, *withRules.default_);
  BOOST_CHECK_EQUAL(1_sz, withRules.rules_.size());
  BOOST_CHECK_EQUAL(ph, withRules.rules_[0].first.front());
  BOOST_CHECK_EQUAL(ph, withRules.rules_[0].second);
}

BOOST_AUTO_TEST_CASE(Router_update_Invalid_Range)
{
  auto router = Router{fn};
  BOOST_CHECK(begin(router) == end(router));
  BOOST_CHECK_EXCEPTION(router.update(ph, {{"Invalid Range"}}), SystemError,
                        verifyException<PichiError::SEMANTIC_ERROR>);
  BOOST_CHECK(begin(router) == end(router));
}

BOOST_AUTO_TEST_CASE(Router_update_Invalid_Type)
{
  auto router = Router{fn};
  BOOST_CHECK(begin(router) == end(router));
  BOOST_CHECK_EXCEPTION(router.update(ph, {{}, {}, {AdapterType::DIRECT}}), SystemError,
                        verifyException<PichiError::SEMANTIC_ERROR>);
  BOOST_CHECK_EXCEPTION(router.update(ph, {{}, {}, {AdapterType::REJECT}}), SystemError,
                        verifyException<PichiError::SEMANTIC_ERROR>);
  BOOST_CHECK(begin(router) == end(router));
}

BOOST_AUTO_TEST_CASE(Router_Matching_Range)
{
  auto router = Router{fn};
  router.update(ph, {{"10.0.0.0/8", "fd00::/8"}});
  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});

  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, createRR("10.0.0.1")) == ph);
  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, createRR("fd00::1")) == ph);
  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, createRR("127.0.0.1")) == vo::type::DIRECT);
  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, createRR("fe00::1")) == vo::type::DIRECT);
}

BOOST_AUTO_TEST_CASE(Router_Matching_Ingress)
{
  auto router = Router{fn};
  router.update(ph, {{}, {ph}});
  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});

  auto r = createRR("fe00::1");
  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, r) == ph);
  BOOST_CHECK(router.route({}, "NotMatched", AdapterType::DIRECT, r) == vo::type::DIRECT);
}

BOOST_AUTO_TEST_CASE(Router_Matching_Type)
{
  auto router = Router{fn};
  router.update(ph, {{}, {}, {AdapterType::HTTP}});
  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});

  auto r = createRR("fe00::1");
  BOOST_CHECK(router.route({}, ph, AdapterType::HTTP, r) == ph);
  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, r) == vo::type::DIRECT);
}

BOOST_AUTO_TEST_CASE(Router_Matching_Pattern)
{
  auto router = Router{fn};
  router.update(ph, {{}, {}, {}, {"^.*\\.example\\.com$"}});
  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});

  auto r = createRR();
  for (auto type : {EndpointType::DOMAIN_NAME, EndpointType::IPV4, EndpointType::IPV6}) {
    BOOST_CHECK(
        router.route({type, "foo.example.com", 0_u16}, ph, AdapterType::DIRECT, createRR()) == ph);
    BOOST_CHECK(router.route({type, "fooexample.com", 0_u16}, ph, AdapterType::DIRECT,
                             createRR()) == vo::type::DIRECT);
  }
}

BOOST_AUTO_TEST_CASE(Router_Matching_Domain)
{
  auto router = Router{fn};
  router.update(ph, {{}, {}, {}, {}, {"example.com"}});
  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});

  BOOST_CHECK(router.route({EndpointType::DOMAIN_NAME, "foo.example.com", 0_u16}, ph,
                           AdapterType::DIRECT, createRR()) == ph);
  BOOST_CHECK(router.route({EndpointType::DOMAIN_NAME, "fooexample.com", 0_u16}, ph,
                           AdapterType::DIRECT, createRR()) == vo::type::DIRECT);
}

BOOST_AUTO_TEST_CASE(Router_Matching_Domain_With_Invalid_Type)
{
  auto router = Router{fn};
  router.update(ph, {{}, {}, {}, {}, {"example.com"}});
  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});

  BOOST_CHECK(router.route({EndpointType::IPV4, "foo.example.com", 0_u16}, ph, AdapterType::DIRECT,
                           createRR()) == vo::type::DIRECT);
  BOOST_CHECK(router.route({EndpointType::IPV6, "foo.example.com", 0_u16}, ph, AdapterType::DIRECT,
                           createRR()) == vo::type::DIRECT);
}

BOOST_AUTO_TEST_CASE(Router_Matching_Country)
{
  auto router = Router{fn};
  router.update(ph, {{}, {}, {}, {}, {}, {"AU"}});
  router.setRoute({{}, {make_pair(vector<string>{ph}, ph)}});

  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, createRR("1.1.1.1")) == ph);
  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, createRR("::ffff:1.1.1.1")) == ph);
  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, createRR("8.8.8.8")) == vo::type::DIRECT);
  BOOST_CHECK(router.route({}, ph, AdapterType::DIRECT, createRR("::ffff:8.8.8.8")) ==
              vo::type::DIRECT);
}

BOOST_AUTO_TEST_CASE(Router_Conditionally_Resolving_Default)
{
  auto router = Router{fn};
  BOOST_CHECK(!router.needResloving());
}

BOOST_AUTO_TEST_CASE(Router_Conditionally_Resolving_Unnecessary_Rules)
{
  auto router = Router{fn};
  router.update(ph, {{}, {ph}, {AdapterType::SS}, {ph}, {ph}, {}});
  router.setRoute({ph, {make_pair(vector<string>{ph}, ph)}});
  BOOST_CHECK(!router.needResloving());
}

BOOST_AUTO_TEST_CASE(Router_Conditionally_Resolving_Unnecessary_Route)
{
  auto router = Router{fn};
  router.update(vo::rule::RANGE, {{"127.0.0.1/32"}});
  router.update(vo::rule::COUNTRY, {{}, {}, {}, {}, {}, {ph}});
  BOOST_CHECK(!router.needResloving());
}

BOOST_AUTO_TEST_CASE(Router_Conditionally_Resolving_Necessary_Range)
{
  auto router = Router{fn};
  router.update(ph, {{"127.0.0.1/32"}});
  router.setRoute({ph, {make_pair(vector<string>{ph}, ph)}});

  BOOST_CHECK(router.needResloving());
}

BOOST_AUTO_TEST_CASE(Router_Conditionally_Resolving_Necessary_Country)
{
  auto router = Router{fn};
  router.update(ph, {{}, {}, {}, {}, {}, {ph}});
  router.setRoute({ph, {make_pair(vector<string>{ph}, ph)}});
  BOOST_CHECK(router.needResloving());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
