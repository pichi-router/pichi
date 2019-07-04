#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common.hpp>
#include <sodium/utils.h>
#include <string.h>

using namespace std;
using namespace pichi::api;
using namespace rapidjson;

namespace pichi {

static auto doc = Document{};
Document::AllocatorType& alloc = doc.GetAllocator();

vector<uint8_t> str2vec(string_view s) { return {cbegin(s), cend(s)}; }

vector<uint8_t> hex2bin(string_view hex)
{
  auto v = vector<uint8_t>(hex.size() / 2, 0);
  sodium_hex2bin(v.data(), v.size(), hex.data(), hex.size(), nullptr, nullptr, nullptr);
  return v;
}

IngressVO defaultIngressVO(AdapterType type)
{
  switch (type) {
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    return {type, ph, 1_u16, {}, {}, {}, false};
  case AdapterType::SS:
    return {AdapterType::SS, ph, 1_u16, CryptoMethod::RC4_MD5, ph};
  default:
    BOOST_ERROR("Invalid type");
    return {};
  }
}

Value defaultIngressJson(AdapterType type)
{
  auto v = Value{};
  v.SetObject();
  v.AddMember("bind", ph, alloc);
  v.AddMember("port", 1, alloc);
  switch (type) {
  case AdapterType::HTTP:
    v.AddMember("type", "http", alloc);
    v.AddMember("tls", false, alloc);
    break;
  case AdapterType::SOCKS5:
    v.AddMember("type", "socks5", alloc);
    v.AddMember("tls", false, alloc);
    break;
  case AdapterType::SS:
    v.AddMember("type", "ss", alloc);
    v.AddMember("method", "rc4-md5", alloc);
    v.AddMember("password", ph, alloc);
    break;
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return v;
}

EgressVO defaultEgressVO(AdapterType type)
{
  switch (type) {
  case AdapterType::DIRECT:
    return {AdapterType::DIRECT};
  case AdapterType::REJECT:
    return {AdapterType::REJECT, {}, {}, {}, {}, DelayMode::FIXED, 0_u16};
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    return {type, ph, 1_u16, {}, {}, {}, {}, {}, false};
  case AdapterType::SS:
    return {AdapterType::SS, ph, 1_u16, CryptoMethod::RC4_MD5, ph};
  default:
    BOOST_ERROR("Invalid type");
    return {};
  }
}

Value defaultEgressJson(AdapterType type)
{
  auto v = Value{};
  v.SetObject();
  if (type != AdapterType::DIRECT && type != AdapterType::REJECT) {
    v.AddMember("host", ph, alloc);
    v.AddMember("port", 1, alloc);
  }
  switch (type) {
  case AdapterType::DIRECT:
    v.AddMember("type", "direct", alloc);
    break;
  case AdapterType::REJECT:
    v.AddMember("type", "reject", alloc);
    v.AddMember("mode", "fixed", alloc);
    v.AddMember("delay", 0, alloc);
    break;
  case AdapterType::HTTP:
    v.AddMember("type", "http", alloc);
    v.AddMember("tls", false, alloc);
    break;
  case AdapterType::SOCKS5:
    v.AddMember("type", "socks5", alloc);
    v.AddMember("tls", false, alloc);
    break;
  case AdapterType::SS:
    v.AddMember("type", "ss", alloc);
    v.AddMember("method", "rc4-md5", alloc);
    v.AddMember("password", ph, alloc);
    break;
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return v;
}

} // namespace pichi
