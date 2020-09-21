#include <boost/algorithm/string.hpp>
#include <cmath>
#include <pichi/common/asserts.hpp>
#include <pichi/common/uri.hpp>
#include <regex>

using namespace std;
using namespace std::string_view_literals;

namespace pichi {

// This URI regex isn't intended to follow RFC3986
static auto const URI_REGEX =
    regex{"^(https?)://(([^:/?#\\[\\]]+)|\\[([a-f0-9:.]+)\\])(:(\\d+))?(/[^#?]*([#?].*)?)?$",
          regex::icase};
static auto const URI_REGEX_SIZE = 9;
static auto const HOST_REGEX = regex{"^(([^:/\\[\\]]+)|\\[([a-f0-9:.]+)\\])(:(\\d+))?$"};
static auto const HOST_REGEX_SIZE = 6;

static string_view r2sv(csub_match const& m, csub_match::difference_type size)
{
  assert(size <= m.length());
  return {m.first, static_cast<size_t>(abs(size))};
}

static string_view r2sv(csub_match const& m) { return r2sv(m, m.length()); }

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
  auto r = matching(s, URI_REGEX, URI_REGEX_SIZE);

  all_ = r2sv(r[0]);
  scheme_ = r2sv(r[1]);
  host_ = r2sv(r[2]);
  host_ = r[3].matched ? r2sv(r[3]) : r2sv(r[4]);
  port_ = r[5].matched ? r2sv(r[6]) : scheme2port(scheme_);
  suffix_ = r[7].matched ? r2sv(r[7]) : "/"sv;
  path_ = r[7].matched ? r2sv(r[7], distance(r[7].first, r[8].first)) : "/"sv;
  query_ = r2sv(r[8]);
}

HostAndPort::HostAndPort(string_view s)
{
  auto r = matching(s, HOST_REGEX, HOST_REGEX_SIZE);
  host_ = r[2].matched ? r2sv(r[2]) : r2sv(r[3]);
  port_ = r[4].matched ? r2sv(r[5]) : "80"sv;
}

}  // namespace pichi
