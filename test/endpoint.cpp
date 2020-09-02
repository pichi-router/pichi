#define BOOST_TEST_MODULE pichi endpoint test

#include "utils.hpp"
#include <algorithm>
#include <array>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <initializer_list>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>

using namespace std;
using namespace pichi;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace sys = boost::system;
namespace mpl = boost::mpl;

using EType = Endpoint::Type;

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

class StubSocket {
public:
  explicit StubSocket() {}

  void write(initializer_list<uint8_t> buf)
  {
    copy_n(cbegin(buf), buf.size(), asio::buffers_begin(buffer_.prepare(buf.size())));
    buffer_.commit(buf.size());
  }

  void write(uint8_t c)
  {
    copy_n(&c, 1, asio::buffers_begin(buffer_.prepare(1)));
    buffer_.commit(1);
  }

  void read(MutableBuffer<uint8_t> buf)
  {
    assertTrue(buffer_.size() >= buf.size());
    copy_n(asio::buffers_begin(buffer_.data()), buf.size(), begin(buf));
    buffer_.consume(buf.size());
    transfered_ += buf.size();
  }

  size_t transfered() const { return transfered_; }

private:
  beast::flat_static_buffer<1024> buffer_;
  size_t transfered_ = 0;
};

BOOST_AUTO_TEST_SUITE(ENDPOINT)

BOOST_AUTO_TEST_CASE(serialize_Empty_Host)
{
  auto buf = array<uint8_t, 64>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::DOMAIN_NAME, ""s, "1"s}, buf), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::IPV4, ""s, "1"s}, buf), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::IPV6, ""s, "1"s}, buf), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(serialize_Empty_Port)
{
  auto buf = array<uint8_t, 64>{};
  for (auto host : {"localhost"sv, "127.0.0.1"sv, "::1"sv}) {
    BOOST_CHECK_EXCEPTION(serializeEndpoint(makeEndpoint(host, ""sv), buf), Exception,
                          verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(serialize_Alphabetic_Port)
{
  auto buf = array<uint8_t, 64>{};
  for (auto host : {"localhost"sv, "127.0.0.1"sv, "::1"sv}) {
    BOOST_CHECK_EXCEPTION(serializeEndpoint(makeEndpoint(host, "http"sv), buf), invalid_argument,
                          [](auto&&) { return true; });
  }
}

BOOST_AUTO_TEST_CASE(serialize_Port_Out_Of_Range)
{
  auto buf = array<uint8_t, 64>{};
  for (auto host : {"localhost"sv, "127.0.0.1"sv, "::1"sv}) {
    BOOST_CHECK_EXCEPTION(serializeEndpoint(makeEndpoint(host, 0), buf), Exception,
                          verifyException<PichiError::MISC>);
    BOOST_CHECK_EXCEPTION(serializeEndpoint(makeEndpoint(host, "65536"sv), buf), Exception,
                          verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(serialize_Invalid_IP_Address)
{
  auto buf = array<uint8_t, 64>{};

  for (auto type : {EType::IPV4, EType::IPV6}) {
    BOOST_CHECK_EXCEPTION(serializeEndpoint({type, "localhost"s, "1"s}, buf), sys::system_error,
                          verifyException<asio::error::invalid_argument>);
  }
}

BOOST_AUTO_TEST_CASE(serialize_Domain_Name_Too_Long)
{
  auto buf = array<uint8_t, 1024>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EType::DOMAIN_NAME, string(0x100, 'a'), "1"s}, buf),
                        Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(serialize_Domain_Lack_Of_Buffer)
{
  auto host = "localhost"sv;

  auto enough = array<uint8_t, 13>{};
  BOOST_CHECK_EQUAL(enough.size(), serializeEndpoint(makeEndpoint(host, 1), enough));

  auto lack = array<uint8_t, 12>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint(makeEndpoint(host, 1), lack), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(serialize_Domain)
{
  auto host = "localhost"sv;
  auto port = 443_u16;
  auto expect = array
#ifndef HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
      <uint8_t, 13>
#endif  // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
      {0x03_u8, 0x09_u8, 0x6c_u8, 0x6f_u8, 0x63_u8, 0x61_u8, 0x6c_u8,
       0x68_u8, 0x6f_u8, 0x73_u8, 0x74_u8, 0x01_u8, 0xbb_u8};

  auto fact = array<uint8_t, 13>{};
  auto len = serializeEndpoint(makeEndpoint(host, port), fact);

  BOOST_CHECK_EQUAL(expect.size(), len);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE(serialize_IPv4_Lack_Of_Buffer)
{
  auto address = "127.0.0.1"sv;

  auto enough = array<uint8_t, 7>{};
  BOOST_CHECK_EQUAL(enough.size(), serializeEndpoint(makeEndpoint(address, 1), enough));

  auto lack = array<uint8_t, 6>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint(makeEndpoint(address, 1), lack), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(serialize_IPv4)
{
  auto address = "127.0.0.1"sv;
  auto port = 443_u16;
  auto expect = array
#ifndef HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
      <uint8_t, 7>
#endif  // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
      {0x01, 0x7f, 0x00, 0x00, 0x01, 0x01, 0xbb};

  auto fact = array<uint8_t, 7>{};
  BOOST_CHECK_EQUAL(expect.size(), serializeEndpoint(makeEndpoint(address, port), fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE(serialize_IPv6_Lack_Of_Buffer)
{
  auto address = "::1"sv;

  auto enough = array<uint8_t, 19>{};
  BOOST_CHECK_EQUAL(enough.size(), serializeEndpoint(makeEndpoint(address, 1), enough));

  auto lack = array<uint8_t, 18>{};
  BOOST_CHECK_EXCEPTION(serializeEndpoint(makeEndpoint(address, 1), lack), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(serialize_IPv6)
{
  auto address = "::1"sv;
  auto port = 443_u16;
  auto expect = array
#ifndef HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
      <uint8_t, 19>
#endif  // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
      {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xbb};

  auto fact = array<uint8_t, 19>{};
  BOOST_CHECK_EQUAL(expect.size(), serializeEndpoint(makeEndpoint(address, port), fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE(parse_Invalid_Type)
{
  for (auto i = 0; i < 0x100; ++i) {
    if (i == 0x01 || i == 0x03 || i == 0x04) continue;
    auto t = static_cast<uint8_t>(i);
    auto stub = StubSocket{};
    stub.write(t);
    BOOST_CHECK_EXCEPTION(parseEndpoint([&stub](auto buf) { stub.read(buf); }), Exception,
                          verifyException<PichiError::BAD_PROTO>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Empty_Domain)
{
  auto stub = StubSocket{};
  stub.write({0x03, 0x00, 0x01, 0xbb});
  BOOST_CHECK_EXCEPTION(parseEndpoint([&stub](auto buf) { stub.read(buf); }), Exception,
                        verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(parse_Domain)
{
  auto stub = StubSocket{};
  stub.write({0x03, 0x09, 'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', 0x01, 0xbb});

  auto ep = parseEndpoint([&stub](auto buf) { stub.read(buf); });
  BOOST_CHECK(EType::DOMAIN_NAME == ep.type_);
  BOOST_CHECK_EQUAL("localhost"sv, ep.host_);
  BOOST_CHECK_EQUAL("443"sv, ep.port_);
  BOOST_CHECK_EQUAL(13_sz, stub.transfered());
}

BOOST_AUTO_TEST_CASE(parse_IPv4)
{
  auto stub = StubSocket{};
  stub.write({0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0xbb});

  auto ep = parseEndpoint([&stub](auto buf) { stub.read(buf); });
  BOOST_CHECK(EType::IPV4 == ep.type_);
  BOOST_CHECK_EQUAL("0.0.0.0"sv, ep.host_);
  BOOST_CHECK_EQUAL("443"sv, ep.port_);
  BOOST_CHECK_EQUAL(7_sz, stub.transfered());
}

BOOST_AUTO_TEST_CASE(parse_IPv6)
{
  auto stub = StubSocket{};
  stub.write({0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x01, 0xbb});

  auto ep = parseEndpoint([&stub](auto buf) { stub.read(buf); });
  BOOST_CHECK(EType::IPV6 == ep.type_);
  BOOST_CHECK_EQUAL("::"sv, ep.host_);
  BOOST_CHECK_EQUAL("443"sv, ep.port_);
  BOOST_CHECK_EQUAL(19_sz, stub.transfered());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(hton_0, Int, IntTypes)
{
  auto expt = array<uint8_t, sizeof(Int)>{};
  auto fact = array<uint8_t, sizeof(Int)>{};

  fill_n(begin(expt), sizeof(Int), 0_u8);
  auto zero = Int{0};
  hton(zero, fact);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expt), cend(expt), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(hton_1, Int, IntTypes)
{
  auto expt = array<uint8_t, sizeof(Int)>{};
  auto fact = array<uint8_t, sizeof(Int)>{};

  fill_n(begin(expt), sizeof(Int), 0xff_u8);
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

  fill_n(begin(buf), sizeof(buf), 0_u8);
  BOOST_CHECK_EQUAL(static_cast<Int>(0), ntoh<Int>(buf));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ntoh_1, Int, IntTypes)
{
  auto buf = array<uint8_t, sizeof(Int)>{};

  fill_n(begin(buf), sizeof(buf), 0xff_u8);
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
  fill_n(begin(fact), sizeof(fact), 0_u8);
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
