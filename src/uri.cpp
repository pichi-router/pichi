#include <pichi/asserts.hpp>
#include <pichi/uri.hpp>
#include <regex>

using namespace std;
using namespace std::string_view_literals;

namespace pichi {

// This URI regex isn't intended to follow RFC3986
static auto const URI_REGEX = regex{"^(https?)://([^:/]+)(:(\\d+))?((/[^#?]*)?([#?].*)?)$"};
static auto const HOST_REGEX = regex{"^([^:/]+)(:(\\d+))?$"};

using MatchResults = match_results<std::string_view::const_iterator>;

template <typename BidirIt> string_view it2sv(BidirIt first, BidirIt last)
{
  return {first, static_cast<string_view::size_type>(distance(first, last))};
}

static auto matching(string_view s, regex const& re, size_t size)
{
  auto r = MatchResults{};
  assertTrue(regex_match(cbegin(s), cend(s), r, re), PichiError::BAD_PROTO);
  assertTrue(r.size() == size, PichiError::BAD_PROTO);
  return r;
}

static string_view scheme2port(string_view scheme)
{
  if (scheme == "http"sv)
    return "80"sv;
  else if (scheme == "https"sv)
    return "443"sv;
  else
    fail(PichiError::BAD_PROTO);
}

Uri::Uri(string_view s)
{
  auto r = matching(s, URI_REGEX, 8);
  all_ = it2sv(r[0].first, r[0].second);
  scheme_ = it2sv(r[1].first, r[1].second);
  host_ = it2sv(r[2].first, r[2].second);
  port_ = it2sv(r[4].first, r[4].second);
  if (port_.empty()) port_ = scheme2port(scheme_);
  suffix_ = it2sv(r[5].first, r[5].second);
  if (suffix_.empty()) suffix_ = "/"sv;
  path_ = r[6].matched ? it2sv(r[6].first, r[6].second) : "/"sv;
  query_ = it2sv(r[7].first, r[7].second);
}

HostAndPort::HostAndPort(string_view s)
{
  auto r = matching(s, HOST_REGEX, 4);
  host_ = it2sv(r[1].first, r[1].second);
  port_ = it2sv(r[3].first, r[3].second);
}

} // namespace pichi
