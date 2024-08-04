#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ip/address.hpp>
#include <charconv>
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
  static constexpr auto BYTES_SIZE = sizeof(BytesType);

  static string bytes2Ip(ConstBuffer bytes)
  {
    auto tmp = BytesType{};
    copy_n(cbegin(bytes), BYTES_SIZE, begin(tmp));
    if constexpr (is_same_v<AddressType, ip::address_v4>)
      return ip::make_address_v4(tmp).to_string();
    else
      return ip::make_address_v6(tmp).to_string();
  }

  static size_t ip2Bytes(string_view ip, MutableBuffer dst)
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

size_t serializeEndpoint(Endpoint const& endpoint, MutableBuffer target)
{
  assertFalse(endpoint.host_.empty());
  auto pos = target.begin();
  switch (endpoint.type_) {
  case EndpointType::DOMAIN_NAME:
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
  case EndpointType::IPV4:
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
  case EndpointType::IPV6:
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
  hton<uint16_t>(endpoint.port_, {pos, sizeof(uint16_t)});
  pos += sizeof(uint16_t);

  return pos - target.begin();
}

Endpoint parseEndpoint(function<void(MutableBuffer)> read)
{
  auto buf = std::array<uint8_t, 512>{};
  auto len = uint8_t{0};

  read({buf, 1});
  switch (buf[0]) {
  case 0x01:
    read({buf, IPv4::BYTES_SIZE + sizeof(uint16_t)});
    return Endpoint{EndpointType::IPV4, IPv4::bytes2Ip(buf),
                    ntoh<uint16_t>({buf.data() + IPv4::BYTES_SIZE, sizeof(uint16_t)})};
  case 0x03:
    read({buf, 1});
    len = buf.front();
    assertTrue(len > 0, PichiError::BAD_PROTO);

    read({buf, static_cast<size_t>(len + 2)});
    return Endpoint{EndpointType::DOMAIN_NAME,
                    {cbegin(buf), cbegin(buf) + len},
                    ntoh<uint16_t>({buf.data() + len, sizeof(uint16_t)})};
  case 0x04:
    read({buf, IPv6::BYTES_SIZE + sizeof(uint16_t)});
    return Endpoint{EndpointType::IPV6, IPv6::bytes2Ip(buf),
                    ntoh<uint16_t>({buf.data() + IPv6::BYTES_SIZE, sizeof(uint16_t)})};
  default:
    fail(PichiError::BAD_PROTO);
  }
}

EndpointType detectHostType(string_view host)
{
  assertFalse(host.empty(), PichiError::MISC);
  auto ec = sys::error_code{};
  auto address = ip::make_address(host, ec);
  if (ec) return EndpointType::DOMAIN_NAME;
  return address.is_v4() ? EndpointType::IPV4 : EndpointType::IPV6;
}

Endpoint makeEndpoint(string_view host, uint16_t port)
{
  return {detectHostType(host), to_string(host), port};
}

Endpoint makeEndpoint(string_view host, string_view port)
{
  auto p = 0;
  auto [ptr, ec] = from_chars(port.data(), port.data() + port.size(), p);
  assertTrue(ec == errc{});
  assertTrue(ptr == port.data() + port.size());
  assertTrue(p >= 0);
  assertTrue(p <= numeric_limits<uint16_t>::max());
  return {detectHostType(host), to_string(host), static_cast<uint16_t>(p)};
}

bool operator==(Endpoint const& lhs, Endpoint const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.host_ == rhs.host_ && lhs.port_ == rhs.port_;
}

}  // namespace pichi
