#define BOOST_TEST_MODULE pichi endpoint test

#include "utils.hpp"
#include <algorithm>
#include <array>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/test/unit_test.hpp>
#include <initializer_list>
#include <pichi/common.hpp>
#include <pichi/net/helpers.hpp>

using namespace std;
using namespace pichi;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace sys = boost::system;

using EndpointType = net::Endpoint::Type;
using net::makeEndpoint;
using net::parseEndpoint;
using net::serializeEndpoint;

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
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EndpointType::DOMAIN_NAME, ""s, "1"s}, buf), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EndpointType::IPV4, ""s, "1"s}, buf), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(serializeEndpoint({EndpointType::IPV6, ""s, "1"s}, buf), Exception,
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

  for (auto type : {EndpointType::IPV4, EndpointType::IPV6}) {
    BOOST_CHECK_EXCEPTION(serializeEndpoint({type, "localhost"s, "1"s}, buf), sys::system_error,
                          verifyException<asio::error::invalid_argument>);
  }
}

BOOST_AUTO_TEST_CASE(serialize_Domain_Name_Too_Long)
{
  auto buf = array<uint8_t, 1024>{};
  BOOST_CHECK_EXCEPTION(
      serializeEndpoint({EndpointType::DOMAIN_NAME, string(0x100, 'a'), "1"s}, buf), Exception,
      verifyException<PichiError::MISC>);
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
#endif // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
      {0x03, 0x09, 'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', 0x01, 0xbb};

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
#endif // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
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
#endif // HAS_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
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
  BOOST_CHECK(EndpointType::DOMAIN_NAME == ep.type_);
  BOOST_CHECK_EQUAL("localhost"sv, ep.host_);
  BOOST_CHECK_EQUAL("443"sv, ep.port_);
  BOOST_CHECK_EQUAL(13_sz, stub.transfered());
}

BOOST_AUTO_TEST_CASE(parse_IPv4)
{
  auto stub = StubSocket{};
  stub.write({0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0xbb});

  auto ep = parseEndpoint([&stub](auto buf) { stub.read(buf); });
  BOOST_CHECK(EndpointType::IPV4 == ep.type_);
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
  BOOST_CHECK(EndpointType::IPV6 == ep.type_);
  BOOST_CHECK_EQUAL("::"sv, ep.host_);
  BOOST_CHECK_EQUAL("443"sv, ep.port_);
  BOOST_CHECK_EQUAL(19_sz, stub.transfered());
}

BOOST_AUTO_TEST_SUITE_END()
