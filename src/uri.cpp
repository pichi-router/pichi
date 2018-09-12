#include <boost/algorithm/string.hpp>
#include <pichi/asserts.hpp>
#include <pichi/uri.hpp>
#include <regex>

using namespace std;
using namespace std::string_view_literals;

namespace pichi {

// This URI regex isn't intended to follow RFC3986
static auto const URI_REGEX =
    regex{"^(https?)://([^:/?#]+)(:(\\d+))?(/[^#?]*([#?].*)?)?$", regex::icase};
static auto const HOST_REGEX = regex{"^([^:/]+):(\\d+)$"};

static string_view r2sv(csub_match const& m)
{
  return {m.first, static_cast<string_view::size_type>(m.length())};
}

static string_view r2sv(csub_match const& m, size_t size)
{
  assert(size <= m.length());
  return {m.first, size};
}

static auto matching(string_view s, regex const& re, size_t size)
{
  auto r = cmatch{};
  assertTrue(regex_match(s.data(), s.data() + s.size(), r, re), PichiError::BAD_PROTO);
  assertTrue(r.size() == size, PichiError::BAD_PROTO);
  return r;
}

static string_view scheme2port(string_view scheme)
{
  if (boost::iequals("http"sv, scheme))
    return "80"sv;
  else if (boost::iequals("https"sv, scheme))
    return "443"sv;
  else
    fail(PichiError::BAD_PROTO);
}

Uri::Uri(string_view s)
{
  auto r = matching(s, URI_REGEX, 7);

  all_ = r2sv(r[0]);
  scheme_ = r2sv(r[1]);
  host_ = r2sv(r[2]);
  port_ = r[3].matched ? r2sv(r[4]) : scheme2port(scheme_);
  suffix_ = r[5].matched ? r2sv(r[5]) : "/"sv;
  path_ = r[5].matched ? r2sv(r[5], distance(r[5].first, r[6].first)) : "/"sv;
  query_ = r2sv(r[6]);
}

HostAndPort::HostAndPort(string_view s)
{
  auto r = matching(s, HOST_REGEX, 3);
  host_ = r2sv(r[1]);
  port_ = r2sv(r[2]);
}

} // namespace pichi
