#define BOOST_TEST_MODULE pichi uri test

#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common/uri.hpp>

using namespace std;
using namespace pichi;

BOOST_AUTO_TEST_SUITE(URI)

BOOST_AUTO_TEST_CASE(Uri_Bad_Scheme)
{
  BOOST_CHECK_EXCEPTION(Uri{"ws://localhost/"}, Exception, verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"wss://localhost/"}, Exception, verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"unknown://localhost/"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(Uri_Scheme)
{
  auto http = Uri{"http://localhost/"};
  BOOST_CHECK_EQUAL("http", http.scheme_);

  auto https = Uri{"https://localhost/"};
  BOOST_CHECK_EQUAL("https", https.scheme_);
}

BOOST_AUTO_TEST_CASE(Uri_Empty_Host)
{
  BOOST_CHECK_EXCEPTION(Uri{"http:///"}, Exception, verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"http://:80/"}, Exception, verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"http://[]/"}, Exception, verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"http://[]:80/"}, Exception, verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(Uri_Host)
{
  auto localhost = Uri{"http://localhost/"};
  BOOST_CHECK_EQUAL("localhost", localhost.host_);

  auto domain = Uri{"http://example.com/"};
  BOOST_CHECK_EQUAL("example.com", domain.host_);
}

BOOST_AUTO_TEST_CASE(Uri_Host_IPv6)
{
  BOOST_CHECK_EQUAL("::", Uri{"http://[::]"}.host_);
  BOOST_CHECK_EQUAL("fe80::1", Uri{"http://[fe80::1]"}.host_);
  BOOST_CHECK_EQUAL("::1", Uri{"http://[::1]"}.host_);
  BOOST_CHECK_EQUAL("::1", Uri{"http://[::1]/"}.host_);
  BOOST_CHECK_EQUAL("::1", Uri{"http://[::1]:80"}.host_);
  BOOST_CHECK_EQUAL("::1", Uri{"http://[::1]:80/"}.host_);
  BOOST_CHECK_EQUAL("::1", Uri{"http://[::1]/path"}.host_);
  BOOST_CHECK_EQUAL("::1", Uri{"http://[::1]:80/path"}.host_);
  BOOST_CHECK_EQUAL("::1", Uri{"http://[::1]/?query"}.host_);
  BOOST_CHECK_EQUAL("::1", Uri{"http://[::1]/path?query"}.host_);
}

BOOST_AUTO_TEST_CASE(Uri_Bad_Port_Field)
{
  BOOST_CHECK_EXCEPTION(Uri{"http://localhost:/"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"http://localhost:alpha/"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"http://localhost:%/"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(Uri_Omit_Port)
{
  auto http = Uri{"http://localhost/"};
  BOOST_CHECK_EQUAL("localhost", http.host_);
  BOOST_CHECK_EQUAL("80", http.port_);

  auto https = Uri{"https://localhost/"};
  BOOST_CHECK_EQUAL("localhost", https.host_);
  BOOST_CHECK_EQUAL("443", https.port_);
}

BOOST_AUTO_TEST_CASE(Uri_Specific_Port)
{
  auto uri = Uri{"http://localhost:0/"};
  BOOST_CHECK_EQUAL("localhost", uri.host_);
  BOOST_CHECK_EQUAL("0", uri.port_);

  uri = {"http://localhost:65536/"};
  BOOST_CHECK_EQUAL("localhost", uri.host_);
  BOOST_CHECK_EQUAL("65536", uri.port_);

  uri = {"http://localhost:1024/"};
  BOOST_CHECK_EQUAL("localhost", uri.host_);
  BOOST_CHECK_EQUAL("1024", uri.port_);
}

BOOST_AUTO_TEST_CASE(Uri_Root_Path)
{
  auto uri = Uri{"http://localhost"};
  BOOST_CHECK_EQUAL("/", uri.suffix_);
  BOOST_CHECK_EQUAL("/", uri.path_);
  BOOST_CHECK(uri.query_.empty());

  uri = Uri{"http://localhost/"};
  BOOST_CHECK_EQUAL("/", uri.suffix_);
  BOOST_CHECK_EQUAL("/", uri.path_);
  BOOST_CHECK(uri.query_.empty());
}

BOOST_AUTO_TEST_CASE(Uri_With_Path)
{
  auto uri = Uri{"http://localhost/path"};
  BOOST_CHECK_EQUAL("/path", uri.path_);

  uri = Uri{"http://localhost/path/"};
  BOOST_CHECK_EQUAL("/path/", uri.path_);
}

BOOST_AUTO_TEST_CASE(Uri_With_Query_Only)
{
  auto uri = Uri{"http://localhost/path?query"};
  BOOST_CHECK_EQUAL("/path?query", uri.suffix_);
  BOOST_CHECK_EQUAL("/path", uri.path_);
  BOOST_CHECK_EQUAL("?query", uri.query_);
}

BOOST_AUTO_TEST_CASE(Uri_With_Fragment_Only)
{
  auto uri = Uri{"http://localhost/path#fragment"};
  BOOST_CHECK_EQUAL("/path#fragment", uri.suffix_);
  BOOST_CHECK_EQUAL("/path", uri.path_);
  BOOST_CHECK_EQUAL("#fragment", uri.query_);
}

BOOST_AUTO_TEST_CASE(Uri_With_Query_And_Fragment)
{
  auto uri = Uri{"http://localhost/path?query#fragment"};
  BOOST_CHECK_EQUAL("/path?query#fragment", uri.suffix_);
  BOOST_CHECK_EQUAL("/path", uri.path_);
  BOOST_CHECK_EQUAL("?query#fragment", uri.query_);

  uri = Uri{"http://localhost/path#fragment?query"};
  BOOST_CHECK_EQUAL("/path#fragment?query", uri.suffix_);
  BOOST_CHECK_EQUAL("/path", uri.path_);
  BOOST_CHECK_EQUAL("#fragment?query", uri.query_);
}

BOOST_AUTO_TEST_CASE(Uri_Empty_Path_Without_Query)
{
  BOOST_CHECK_EXCEPTION(Uri{"http://localhost?query"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"http://localhost#fragment"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"http://localhost?query#fragment"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(Uri{"http://localhost#fragment?query"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(Uri_Capital_Scheme)
{
  auto http = Uri{"HTTP://localhost/"};
  BOOST_CHECK_EQUAL("HTTP", http.scheme_);
  BOOST_CHECK_EQUAL("80", http.port_);

  auto https = Uri{"HTTPS://localhost/"};
  BOOST_CHECK_EQUAL("HTTPS", https.scheme_);
  BOOST_CHECK_EQUAL("443", https.port_);
}

BOOST_AUTO_TEST_CASE(HostAndPort_Empty_Host)
{
  BOOST_CHECK_EXCEPTION(HostAndPort{""}, Exception, verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(HostAndPort{"[]"}, Exception, verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(HostAndPort{":80"}, Exception, verifyException<PichiError::BAD_PROTO>);
  BOOST_CHECK_EXCEPTION(HostAndPort{"[]:80"}, Exception, verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(HostAndPort_Missing_Port)
{
  auto domain = HostAndPort{"localhost"sv};
  BOOST_CHECK_EQUAL("localhost"sv, domain.host_);
  BOOST_CHECK_EQUAL("80"sv, domain.port_);

  auto ipv4 = HostAndPort{"127.0.0.1"sv};
  BOOST_CHECK_EQUAL("127.0.0.1"sv, ipv4.host_);
  BOOST_CHECK_EQUAL("80"sv, ipv4.port_);

  auto ipv6 = HostAndPort{"[fe80::1]"sv};
  BOOST_CHECK_EQUAL("fe80::1"sv, ipv6.host_);
  BOOST_CHECK_EQUAL("80"sv, ipv6.port_);
}

BOOST_AUTO_TEST_CASE(HostAndPort_Alpha_Port)
{
  BOOST_CHECK_EXCEPTION(HostAndPort{"localhost:http"}, Exception,
                        verifyException<PichiError::BAD_PROTO>);
}

BOOST_AUTO_TEST_CASE(HostAndPort_Normal)
{
  auto hp = HostAndPort{"example.com:443"};
  BOOST_CHECK_EQUAL("example.com", hp.host_);
  BOOST_CHECK_EQUAL("443", hp.port_);
}

BOOST_AUTO_TEST_CASE(HostAndPort_IPv6)
{
  BOOST_CHECK_EQUAL("::", HostAndPort{"[::]:80"}.host_);
  BOOST_CHECK_EQUAL("fe80::1", HostAndPort{"[fe80::1]:80"}.host_);
  BOOST_CHECK_EQUAL("::1", HostAndPort{"[::1]:80"}.host_);
}

BOOST_AUTO_TEST_SUITE_END()
