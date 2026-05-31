#define BOOST_TEST_MODULE pichi router test

#include "utils.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/actor/router.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/error.hpp>
#include <pichi/service/mmdb.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/rule.hpp>
#include <ranges>
#include <string_view>

using namespace std::literals;
namespace asio = boost::asio;
namespace ip   = asio::ip;
namespace rngs = std::ranges;
namespace sys  = boost::system;

using ResolveResults = asio::ip::basic_resolver_results<asio::ip::tcp>;

namespace pichi::unit_test {

static auto const RESOLV_ERROR  = make_error_code(PichiError::MISC);
static auto const SPEC_EGRESS   = "spec"s;
static auto const DEFT_EGRESS   = "deft"s;
static auto const SPEC_RULE     = "spec_rule"s;
static auto const DEFT_RULE     = "*"s;
static auto const STUB_ENDPOINT = Endpoint{.type_ = EndpointType::DOMAIN_NAME};
static auto const EGRESSES      = std::unordered_map<std::string, vo::Egress>{
    {SPEC_EGRESS, {}},
    {DEFT_EGRESS, {}},
};
static auto const RULES = std::unordered_map<std::string, vo::Rule>{
    {SPEC_RULE, {}}
};
static auto const ROUTE = vo::Route{
    .default_ = DEFT_EGRESS,
    .rules_   = {std::make_pair(std::vector<std::string>{SPEC_RULE}, SPEC_EGRESS)},
};

static Awaitable<ResolveResults> resolve(std::string_view rr)
{
  // Prohibit DNS resolving if no specified record
  assertFalse(rngs::empty(rr));
  co_return ResolveResults::create(ip::tcp::endpoint{ip::make_address(rr), 0}, "", "");
}

template <typename Arg>
requires(
    std::same_as<Arg, Endpoint> || std::same_as<Arg, std::string> || std::same_as<Arg, AdapterType>
)
Awaitable<std::tuple<std::string, std::string, vo::Egress>>
    route(IOExecutor const& ex, vo::Rule const& rule, Arg const& arg, std::string_view rr)
{
  auto& mmdb = asio::use_service<service::Mmdb>(asio::query(ex, asio::execution::context));
  mmdb.initialize("geo.mmdb"s);

  auto rules       = RULES;
  rules[SPEC_RULE] = rule;
  auto router      = actor::Router{ex, EGRESSES, rules, ROUTE};

  if constexpr (std::same_as<Arg, Endpoint>)
    co_return co_await router.route(arg, ""s, AdapterType::DIRECT, resolve(rr));
  else if constexpr (std::same_as<Arg, std::string>)
    co_return co_await router.route(STUB_ENDPOINT, arg, AdapterType::DIRECT, resolve(rr));
  else
    co_return co_await router.route(STUB_ENDPOINT, ""s, arg, resolve(rr));
}

template <typename Arg>
void run_generic_test(
    bool matched, vo::Rule const& rule, Arg const& arg, std::string_view rr = ""sv
)
{
  run_case([=](auto&& ex) -> Awaitable<void> {
    auto [r, e, _] = co_await route(ex, rule, arg, rr);
    if (matched) {
      BOOST_CHECK_EQUAL(SPEC_RULE, r);
      BOOST_CHECK_EQUAL(SPEC_EGRESS, e);
    }
    else {
      BOOST_CHECK_EQUAL(DEFT_RULE, r);
      BOOST_CHECK_EQUAL(DEFT_EGRESS, e);
    }
  });
}

template <typename Arg>
void run_generic_test(
    sys::error_code const& expect, vo::Rule const& rule, Arg const& arg, std::string_view rr = ""sv
)
{
  run_case([=](auto&& ex) -> Awaitable<void> {
    auto [fact, _] = co_await redirect(route(ex, rule, arg, rr));
    BOOST_CHECK(expect == fact);
  });
}

static void run_domain_test(bool matched, std::string const& sub, std::string const& domain)
{
  run_generic_test(
      matched,
      vo::Rule{.domain_ = {domain}},
      Endpoint{.type_ = EndpointType::DOMAIN_NAME, .host_ = sub, .port_ = 0}
  );
}

template <typename Expect>
void run_range_test(
    Expect expect, std::string const& host, std::string const& network, std::string_view rr = ""sv
)
{
  run_generic_test(expect, vo::Rule{.range_ = {network}}, makeEndpoint(host, 0), rr);
}

static void run_range_nr_test(
    bool matched, std::string const& host, std::string const& network, std::string_view rr = ""sv
)
{
  run_generic_test(matched, vo::Rule{.range_nr_ = {network}}, makeEndpoint(host, 0), rr);
}

static void run_pattern_test(bool matched, std::string const& name, std::string const& pattern)
{
  run_generic_test(
      matched,
      vo::Rule{.pattern_ = {pattern}},
      Endpoint{.type_ = EndpointType::DOMAIN_NAME, .host_ = name, .port_ = 0}
  );
}

static void run_ingress_test(bool matched, std::string const& name, std::string const& ingress)
{
  run_generic_test(matched, vo::Rule{.ingress_ = {ingress}}, name);
}

static void run_type_test(bool matched, AdapterType t, AdapterType type)
{
  run_generic_test(matched, vo::Rule{.type_ = {type}}, t);
}

template <typename Expect>
void run_country_test(
    Expect expect, std::string const& host, std::string const& country, std::string_view rr = ""sv
)
{
  run_generic_test(expect, vo::Rule{.country_ = {country}}, makeEndpoint(host, 0), rr);
}

static void run_country_nr_test(
    bool matched, std::string const& host, std::string const& country, std::string_view rr = ""sv
)
{
  run_generic_test(matched, vo::Rule{.country_nr_ = {country}}, makeEndpoint(host, 0), rr);
}

BOOST_AUTO_TEST_SUITE(ROUTER)

BOOST_AUTO_TEST_CASE(Matcher_match_Domains_Empty)
{
  run_domain_test(false, "example.com", "");
  run_domain_test(false, "", "example.com");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Domains_Starting_With_Dot)
{
  run_domain_test(false, ".", "com");
  run_domain_test(true, ".com", "com");
  run_domain_test(false, "example.com", ".");
  run_domain_test(true, "example.com", ".com");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Domains_Matched)
{
  run_domain_test(true, "foo.bar.example.com", "bar.example.com");
  run_domain_test(true, "foo.bar.example.com", "example.com");
  run_domain_test(true, "foo.bar.example.com", "com");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Domains_Same_End)
{
  run_domain_test(false, "foobar.example.com", "bar.example.com");
  run_domain_test(false, "foobarexample.com", "example.com");
  run_domain_test(false, "example.com", "m");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Domains_Not_Matched)
{
  run_domain_test(false, "example.com", "example");
  run_domain_test(false, "foo.example.com", "example");
  run_domain_test(false, "example.com", "e.c");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Domains_Same)
{
  run_domain_test(true, "example.com", "example.com");
  run_domain_test(true, "foo.example.com", "foo.example.com");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Domains_Case_Insensitive)
{
  run_domain_test(true, "EXAMPLE.COM", "com");
  run_domain_test(true, "example.com", "COM");
  run_domain_test(true, "Example.com", "Com");
  run_domain_test(true, "EXAMPLE.COM", "example.com");
  run_domain_test(true, "example.com", "EXAMPLE.COM");
  run_domain_test(true, "Example.com", "example.Com");
}

BOOST_AUTO_TEST_CASE(Router_Router_Bad_Input)
{
  auto io = asio::io_context{};
  auto ex = io.get_executor();
  BOOST_CHECK_EXCEPTION(
      actor::Router(ex, {}, RULES, ROUTE),
      SystemError,
      verify_exception<PichiError::MISC>
  );
  BOOST_CHECK_EXCEPTION(
      actor::Router(ex, EGRESSES, {}, ROUTE),
      SystemError,
      verify_exception<PichiError::MISC>
  );
  BOOST_CHECK_THROW(
      actor::Router(ex, EGRESSES, RULES, {.default_ = "Non-existent egress"}),
      std::out_of_range
  );
}

BOOST_AUTO_TEST_CASE(Matcher_match_Ranges)
{
  run_range_test(true, "10.1.1.1", "10.0.0.0/8");
  run_range_test(true, "fd00::1", "fd00::/8");
  run_range_test(false, "127.0.0.1", "10.0.0.0/8");
  run_range_test(false, "fe00::1", "fd00::/8");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Ranges_With_Resolving)
{
  run_range_test(RESOLV_ERROR, "example.com", "10.0.0.0/8");
  run_range_test(RESOLV_ERROR, "example.com", "fd00::/8");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Ranges_With_Resolved_Results)
{
  run_range_test(true, "example.com", "10.0.0.0/8", "10.1.1.1");
  run_range_test(false, "example.com", "10.0.0.0/8", "127.0.0.1");
  run_range_test(false, "example.com", "10.0.0.0/8", "fe80::1");
  run_range_test(true, "example.com", "fd00::/8", "fd00::1");
  run_range_test(false, "example.com", "fd00::/8", "fe80::1");
  run_range_test(false, "example.com", "fd00::/8", "127.0.0.1");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Ranges_NR)
{
  run_range_nr_test(true, "10.1.1.1", "10.0.0.0/8");
  run_range_nr_test(true, "fd00::1", "fd00::/8");
  run_range_nr_test(false, "127.0.0.1", "10.0.0.0/8");
  run_range_nr_test(false, "fe00::1", "fd00::/8");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Ranges_NR_With_Resolving)
{
  run_range_nr_test(false, "example.com", "10.0.0.0/8");
  run_range_nr_test(false, "example.com", "fd00::/8");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Ranges_NR_With_Resolved_Results)
{
  run_range_nr_test(false, "example.com", "10.0.0.0/8", "10.1.1.1");
  run_range_nr_test(false, "example.com", "fd00::/8", "fd00::1");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Ingresses_Empty)
{
  run_ingress_test(true, "", "");
  run_ingress_test(false, "", "foo");
  run_ingress_test(false, "foo", "");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Ingresses)
{
  run_ingress_test(true, "foo", "foo");
  run_ingress_test(false, "foo", "bar");
  run_ingress_test(false, "foo", "foofoo");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Types)
{
  run_type_test(true, AdapterType::HTTP, AdapterType::HTTP);
  run_type_test(false, AdapterType::HTTP, AdapterType::DIRECT);
}

BOOST_AUTO_TEST_CASE(Matcher_match_Patterns)
{
  run_pattern_test(true, "foo.example.com", "^.*\\.example\\.com$");
  run_pattern_test(false, "fooexample.com", "^.*\\.example\\.com$");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Countries)
{
  run_country_test(true, "1.1.1.1", "AU");
  run_country_test(true, "::ffff:1.1.1.1", "AU");
  run_country_test(false, "8.8.8.8", "AU");
  run_country_test(false, "::ffff:8.8.8.8", "AU");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Countries_With_Resolving)
{
  run_country_test(RESOLV_ERROR, "example.com", "AU");
  run_country_test(RESOLV_ERROR, "example.com", "AU");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Countries_With_Resolved_Results)
{
  run_country_test(true, "example.com", "AU", "1.1.1.1");
  run_country_test(true, "example.com", "AU", "::ffff:1.1.1.1");
  run_country_test(false, "example.com", "AU", "8.8.8.8");
  run_country_test(false, "example.com", "AU", "::ffff:8.8.8.8");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Countries_NR)
{
  run_country_nr_test(true, "1.1.1.1", "AU");
  run_country_nr_test(true, "::ffff:1.1.1.1", "AU");
  run_country_nr_test(false, "8.8.8.8", "AU");
  run_country_nr_test(false, "::ffff:8.8.8.8", "AU");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Countries_NR_With_Resolving)
{
  run_country_nr_test(false, "example.com", "AU");
  run_country_nr_test(false, "example.com", "AU");
}

BOOST_AUTO_TEST_CASE(Matcher_match_Countries_NR_With_Resolved_Results)
{
  run_country_nr_test(false, "example.com", "AU", "1.1.1.1");
  run_country_nr_test(false, "example.com", "AU", "::ffff:1.1.1.1");
  run_country_nr_test(false, "example.com", "AU", "8.8.8.8");
  run_country_nr_test(false, "example.com", "AU", "::ffff:8.8.8.8");
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
