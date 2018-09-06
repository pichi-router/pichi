#define BOOST_TEST_MODULE pichi net_helpers test

#include "utils.hpp"
#include <array>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <pichi/net/helpers.hpp>
#include <system_error>

using namespace std;
using namespace pichi;
using namespace pichi::net;
namespace asio = boost::asio;
namespace mpl = boost::mpl;
namespace sys = boost::system;
using EType = Endpoint::Type;

static bool operator==(Endpoint const& lhs, Endpoint const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.host_ == rhs.host_ && lhs.port_ == rhs.port_;
}

static bool operator!=(Endpoint const& lhs, Endpoint const& rhs) { return !(lhs == rhs); }

template <EType type> struct EHelper {
  static uint8_t const CHAR;
  static size_t const SIZE;
  static Endpoint const ENDPOINT;
};

template <> uint8_t const EHelper<EType::DOMAIN_NAME>::CHAR = 3;
template <> size_t const EHelper<EType::DOMAIN_NAME>::SIZE = 7;
template <>
Endpoint const EHelper<EType::DOMAIN_NAME>::ENDPOINT = {EType::DOMAIN_NAME, string(CHAR, CHAR),
                                                        "771"};

template <> uint8_t const EHelper<EType::IPV4>::CHAR = 1;
template <> size_t const EHelper<EType::IPV4>::SIZE = 7;
template <> Endpoint const EHelper<EType::IPV4>::ENDPOINT = {EType::IPV4, "1.1.1.1", "257"};

template <> uint8_t const EHelper<EType::IPV6>::CHAR = 4;
template <> size_t const EHelper<EType::IPV6>::SIZE = 19;
template <>
Endpoint const EHelper<EType::IPV6>::ENDPOINT = {EType::IPV6, "404:404:404:404:404:404:404:404",
                                                 "1028"};

using Helpers = mpl::list<EHelper<EType::DOMAIN_NAME>, EHelper<EType::IPV4>, EHelper<EType::IPV6>>;
using IntTypes =
    mpl::list<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t>;

BOOST_AUTO_TEST_SUITE(NET_HELPERS)

BOOST_AUTO_TEST_CASE_TEMPLATE(hton_0, Int, IntTypes)
{
  auto expt = array<uint8_t, sizeof(Int)>{};
  auto fact = array<uint8_t, sizeof(Int)>{};

  fill_n(begin(expt), sizeof(Int), 0);
  auto zero = Int{0};
  hton(zero, fact);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expt), cend(expt), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(hton_1, Int, IntTypes)
{
  auto expt = array<uint8_t, sizeof(Int)>{};
  auto fact = array<uint8_t, sizeof(Int)>{};

  fill_n(begin(expt), sizeof(Int), 0xff);
  auto ff = static_cast<Int>(~0);
  hton(ff, fact);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expt), cend(expt), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(hton_Seq, Int, IntTypes)
{
  auto expt = array<uint8_t, sizeof(Int)>{};
  auto fact = array<uint8_t, sizeof(Int)>{};

  auto seq = Int{0};
  for (uint8_t i = 0; i < sizeof(Int); ++i) {
    auto p = reinterpret_cast<uint8_t*>(&seq);
    p[i] = expt[sizeof(Int) - i - 1] = i;
  }
  hton(seq, fact);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expt), cend(expt), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ntoh_0, Int, IntTypes)
{
  auto buf = array<uint8_t, sizeof(Int)>{};

  fill_n(begin(buf), sizeof(buf), 0);
  BOOST_CHECK_EQUAL(0, ntoh<Int>(buf));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ntoh_1, Int, IntTypes)
{
  auto buf = array<uint8_t, sizeof(Int)>{};

  fill_n(begin(buf), sizeof(buf), 0xff);
  BOOST_CHECK_EQUAL(static_cast<Int>(~0), ntoh<Int>(buf));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ntoh_Seq, Int, IntTypes)
{
  auto expt = Int{0};
  auto buf = array<uint8_t, sizeof(Int)>{};
  for (uint8_t i = 0; i < sizeof(Int); ++i) {
    auto p = reinterpret_cast<uint8_t*>(&expt);
    p[sizeof(Int) - i - 1] = buf[i] = i;
  }
  BOOST_CHECK_EQUAL(expt, ntoh<Int>(buf));
}

BOOST_AUTO_TEST_CASE(serializeEndpoint_Empty_Endpoint)
{
  auto buf = array<uint8_t, 1024>{};
  for (auto type : {EType::DOMAIN_NAME, EType::IPV4, EType::IPV6}) {
    BOOST_CHECK_EXCEPTION(serializeEndpoint({type, "", ""}, buf), Exception,
                          verifyException<PichiError::MISC>);
    BOOST_CHECK_EXCEPTION(serializeEndpoint({type, "", "placeholder"}, buf), Exception,
                          verifyException<PichiError::MISC>);
    BOOST_CHECK_EXCEPTION(serializeEndpoint({type, "placeholder", ""}, buf), Exception,
                          verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(serializeEndpoint_Invalid_Port_String)
{
  auto buf = array<uint8_t, 1024>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::DOMAIN_NAME, "placeholder", "invalid port"}, buf),
                        invalid_argument,
                        [](auto& e) { return string_view{e.what()}.find("stoi") != string::npos; });
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::IPV4, "127.0.0.1", "invalid port"}, buf),
                        invalid_argument,
                        [](auto& e) { return string_view{e.what()}.find("stoi") != string::npos; });
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::IPV6, "::1", "invalid port"}, buf),
                        invalid_argument,
                        [](auto& e) { return string_view{e.what()}.find("stoi") != string::npos; });
}

BOOST_AUTO_TEST_CASE(serializeEndpoint_Invalid_Port_Range)
{
  auto buf = array<uint8_t, 1024>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::DOMAIN_NAME, "placeholder", "-1"}, buf),
                        Exception, verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::DOMAIN_NAME, "placeholder", "0"}, buf), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::DOMAIN_NAME, "placeholder", "65536"}, buf),
                        Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(serializeEndpoint_Invalid_IP_Address)
{
  auto buf = array<uint8_t, 1024>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::IPV4, "::1", "80"}, buf), sys::system_error,
                        verifyException<asio::error::invalid_argument>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::IPV4, "invalid ipv4 address", "80"}, buf),
                        sys::system_error, verifyException<asio::error::invalid_argument>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::IPV6, "1.1.1.1", "80"}, buf), sys::system_error,
                        verifyException<asio::error::invalid_argument>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::IPV6, "invalid ipv6 address", "80"}, buf),
                        sys::system_error, verifyException<asio::error::invalid_argument>);
}

BOOST_AUTO_TEST_CASE(serializeEndpoint_Invalid_Domain_Length)
{
  auto buf = array<uint8_t, 1024>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::DOMAIN_NAME, string(0x100, 'a'), "80"}, buf),
                        Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(serializeEndpoint_Less_Buffer, Helper, Helpers)
{
  auto buf = array<uint8_t, 1024>{};
  serializeEndpoint(Helper::ENDPOINT, {buf, Helper::SIZE});
  serializeEndpoint(Helper::ENDPOINT, {buf, Helper::SIZE + 1});
  BOOST_CHECK_EXCEPTION(serializeEndpoint(Helper::ENDPOINT, {}), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint(Helper::ENDPOINT, {buf, Helper::SIZE - 1}), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(serializeEndpoint_Normal, Helper, Helpers)
{
  auto expt = array<uint8_t, Helper::SIZE>{};
  fill_n(begin(expt), sizeof(expt), Helper::CHAR);

  auto fact = array<uint8_t, Helper::SIZE>{};
  fill_n(begin(fact), sizeof(fact), 0);
  BOOST_CHECK_EQUAL(sizeof(expt), serializeEndpoint(Helper::ENDPOINT, fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expt), cend(expt), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(parseEndpoint_Normal, Helper, Helpers)
{
  auto read = [size = size_t{0}](MutableBuffer<uint8_t> buf) mutable {
    size += buf.size();
    BOOST_CHECK_LE(size, Helper::SIZE);
    fill_n(begin(buf), Helper::SIZE, Helper::CHAR);
  };
  BOOST_CHECK(Helper::ENDPOINT == parseEndpoint(read));
}

BOOST_AUTO_TEST_CASE(detectHostType_Normal)
{
  BOOST_CHECK_EXCEPTION(detectHostType(""sv), Exception, verifyException<PichiError::MISC>);

  BOOST_CHECK(EType::DOMAIN_NAME == detectHostType("localhost"));

  BOOST_CHECK(EType::IPV4 == detectHostType("0.0.0.0"));
  BOOST_CHECK(EType::IPV4 == detectHostType("127.0.0.1"));

  BOOST_CHECK(EType::IPV6 == detectHostType("::"));
  BOOST_CHECK(EType::IPV6 == detectHostType("fe80::1"));
}

BOOST_AUTO_TEST_SUITE_END()
