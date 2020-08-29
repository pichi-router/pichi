#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ip/address.hpp>
#include <limits>
#include <pichi/common/asserts.hpp>
#include <pichi/common/endpoint.hpp>

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
namespace sys = boost::system;

namespace pichi {

template <typename AddressType> struct AddressHelper {
  static_assert(is_same_v<AddressType, ip::address_v4> || is_same_v<AddressType, ip::address_v6>);
  using BytesType = typename AddressType::bytes_type;
  static auto const BYTES_SIZE = sizeof(BytesType);

  static string bytes2Ip(ConstBuffer<uint8_t> bytes)
  {
    auto tmp = BytesType{};
    copy_n(cbegin(bytes), BYTES_SIZE, begin(tmp));
    if constexpr (is_same_v<AddressType, ip::address_v4>)
      return ip::make_address_v4(tmp).to_string();
    else
      return ip::make_address_v6(tmp).to_string();
  }

  static size_t ip2Bytes(string_view ip, MutableBuffer<uint8_t> dst)
  {
    if constexpr (is_same_v<AddressType, ip::address_v4>)
      copy_n(cbegin(ip::make_address_v4(ip).to_bytes()), BYTES_SIZE, begin(dst));
    else
      copy_n(cbegin(ip::make_address_v6(ip).to_bytes()), BYTES_SIZE, begin(dst));
    return BYTES_SIZE;
  }
};

using IPv4 = AddressHelper<ip::address_v4>;
using IPv6 = AddressHelper<ip::address_v6>;

static string bytes2Port(ConstBuffer<uint8_t> bytes)
{
  return to_string(ntoh<uint16_t>({bytes, sizeof(uint16_t)}));
}

static size_t port2Bytes(string const& port, MutableBuffer<uint8_t> dst)
{
  auto i = stoi(port);
  assertTrue(i > 0, PichiError::MISC);
  assertTrue(i <= numeric_limits<uint16_t>::max(), PichiError::MISC);
  hton(static_cast<uint16_t>(i), dst);
  return sizeof(uint16_t);
}

size_t serializeEndpoint(Endpoint const& endpoint, MutableBuffer<uint8_t> target)
{
  assertFalse(endpoint.host_.empty(), PichiError::MISC);
  assertFalse(endpoint.port_.empty(), PichiError::MISC);
  auto pos = target.begin();
  switch (endpoint.type_) {
  case Endpoint::Type::DOMAIN_NAME:
    /* Format
      +------+----------+----------+------+
      | ATYP | HOST LEN | HOSTNAME | PORT |
      +------+----------+----------+------+
      |  1   |     1    | Variable |  2   |
      +------+----------+----------+------+
     */
    assertTrue(endpoint.host_.size() <= 0xff, PichiError::MISC);
    assertTrue(target.size() >= 4 + endpoint.host_.size(), PichiError::MISC);
    *pos++ = 0x03;
    *pos++ = static_cast<uint8_t>(endpoint.host_.size());
    pos = copy_n(cbegin(endpoint.host_), endpoint.host_.size(), pos);
    break;
  case Endpoint::Type::IPV4:
    /* Format
      +------+-----------+------+
      | ATYP | IPv4 ADDR | PORT |
      +------+-----------+------+
      |  1   |     4     |  2   |
      +------+-----------+------+
     */
    assertTrue(target.size() >= 7, PichiError::MISC);
    *pos++ = 0x01;
    pos += IPv4::ip2Bytes(endpoint.host_, {pos, IPv4::BYTES_SIZE});
    break;
  case Endpoint::Type::IPV6:
    /* Format
      +------+-----------+------+
      | ATYP | IPv4 ADDR | PORT |
      +------+-----------+------+
      |  1   |    16     |  2   |
      +------+-----------+------+
     */
    assertTrue(target.size() >= 19, PichiError::MISC);
    *pos++ = 0x04;
    pos += IPv6::ip2Bytes(endpoint.host_, {pos, IPv6::BYTES_SIZE});
    break;
  default:
    fail(PichiError::BAD_PROTO);
  }
  pos += port2Bytes(endpoint.port_, {pos, sizeof(uint16_t)});

  return pos - target.begin();
}

Endpoint parseEndpoint(function<void(MutableBuffer<uint8_t>)> read)
{
  auto buf = std::array<uint8_t, 512>{};
  auto len = uint8_t{0};

  read({buf, 1});
  switch (buf[0]) {
  case 0x01:
    read({buf, IPv4::BYTES_SIZE + sizeof(uint16_t)});
    return Endpoint{Endpoint::Type::IPV4, IPv4::bytes2Ip(buf),
                    bytes2Port({buf.data() + IPv4::BYTES_SIZE, sizeof(uint16_t)})};
  case 0x03:
    read({buf, 1});
    len = buf.front();
    assertTrue(len > 0, PichiError::BAD_PROTO);

    read({buf, static_cast<size_t>(len + 2)});
    return Endpoint{Endpoint::Type::DOMAIN_NAME,
                    {cbegin(buf), cbegin(buf) + len},
                    bytes2Port({buf.data() + len, sizeof(uint16_t)})};
  case 0x04:
    read({buf, IPv6::BYTES_SIZE + sizeof(uint16_t)});
    return Endpoint{Endpoint::Type::IPV6, IPv6::bytes2Ip(buf),
                    bytes2Port({buf.data() + IPv6::BYTES_SIZE, sizeof(uint16_t)})};
  default:
    fail(PichiError::BAD_PROTO);
  }
}

Endpoint::Type detectHostType(string_view host)
{
  assertFalse(host.empty(), PichiError::MISC);
  auto ec = sys::error_code{};
  auto address = ip::make_address(host, ec);
  if (ec) return Endpoint::Type::DOMAIN_NAME;
  return address.is_v4() ? Endpoint::Type::IPV4 : Endpoint::Type::IPV6;
}

Endpoint makeEndpoint(string_view host, uint16_t port)
{
  return {detectHostType(host), to_string(host), to_string(port)};
}

Endpoint makeEndpoint(string_view host, string_view port)
{
  return {detectHostType(host), to_string(host), to_string(port)};
}

}  // namespace pichi
