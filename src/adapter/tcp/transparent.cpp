#include "pichi/common/config.hpp"
#include <algorithm>
#include <pichi/adapter/tcp/transparent.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/helpers.hpp>

#ifdef TRANSPARENT_PF

extern "C" {
#include <fcntl.h>
#include <net/pfvar.h>
#include <sys/ioctl.h>
#include <unistd.h>
}

#endif  // TRANSPARENT_PF

#ifdef TRANSPARENT_IPTABLES

extern "C" {
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6/ip6_tables.h>
#include <netinet/in.h>
#include <sys/socket.h>
}

#endif  // TRANSPARENT_IPTABLES

namespace asio = boost::asio;
namespace ip   = asio::ip;
namespace rngs = std::ranges;
namespace sys  = boost::system;

namespace pichi::adapter::tcp {

namespace platform {

class AddressHelper {
private:
  static bool is_v6(ip::address const& addr)
  {
    if (addr.is_v4() || (addr.is_v6() && addr.to_v6().is_v4_mapped()))
      return false;
    else if (addr.is_v6())
      return true;
    else
      fail();
  }

  template <typename Address, typename T> void a2b(Address const& src, T* dst)
  {
    rngs::copy(src.to_bytes(), reinterpret_cast<uint8_t*>(dst));
  }

  template <typename Address, typename T> Address b2a(T* src)
  {
    using Bytes = typename Address::bytes_type;
    auto dst    = Bytes{};
    rngs::copy(ConstBuffer{reinterpret_cast<uint8_t*>(src), sizeof(Bytes)}, rngs::begin(dst));
    return Address{dst};
  }

public:
  AddressHelper(ip::address const& addr) : is_v6_{AddressHelper::is_v6(addr)} {}

  template <typename T> void address2buf(ip::address const& src, T* dst)
  {
    assertTrue(is_v6_ == AddressHelper::is_v6(src));
    if (is_v6_)
      a2b(src.to_v6(), dst);
    else
      a2b(src.to_v4(), dst);
  }

  template <typename T> asio::ip::address buf2address(T* src)
  {
    if (is_v6_)
      return b2a<asio::ip::address_v6>(src);
    else
      return b2a<asio::ip::address_v4>(src);
  }

  int family() { return is_v6_ ? AF_INET6 : AF_INET; }

private:
  bool is_v6_;
};

#ifdef TRANSPARENT_PF

static decltype(auto) PF_DEVICE = "/dev/pf";

class PfReader {
public:
  PfReader() : pf_{::open(PF_DEVICE, O_RDONLY)}
  {
    assertTrue(pf_ != -1, "Unable to open PF device");
  }

  ~PfReader() { ::close(pf_); }

  Endpoint read_remote(ip::tcp::endpoint const& src, ip::tcp::endpoint const& dst)
  {
    auto helper = AddressHelper{src.address()};
    auto pnl    = pfioc_natlook{};
    rngs::fill(MutableBuffer{reinterpret_cast<uint8_t*>(&pnl), sizeof(pnl)}, 0_u8);
    pnl.direction = PF_OUT;
    pnl.af        = helper.family();
    pnl.proto     = IPPROTO_TCP;
    helper.address2buf(src.address(), pnl.saddr.addr8);
    helper.address2buf(dst.address(), pnl.daddr.addr8);

#ifdef __APPLE__
    pnl.sxport.port = htons(src.port());
    pnl.dxport.port = htons(dst.port());
#else   // __APPLE__
    pnl.sport = htons(src.port());
    pnl.dport = htons(dst.port());
#endif  // __APPLE__

    assertTrue(ioctl(pf_, DIOCNATLOOK, &pnl) != -1, "Unable to read PF status table");

    return makeEndpoint(
        helper.buf2address(pnl.rdaddr.addr8).to_string(),
#ifdef __APPLE__
        ntohs(pnl.rdxport.port)
#else   // __APPLE__
        ntohs(pnl.rdport)
#endif  // __APPLE__
    );
  }

private:
  int pf_;
};

#endif  // TRANSPARENT_PF

#ifdef TRANSPARENT_IPTABLES

static Endpoint read_remote(ip::tcp::socket& s)
{
  auto helper = AddressHelper{s.remote_endpoint().address()};
  if (helper.family() == AF_INET) {
    auto sa  = sockaddr_in{};
    auto len = socklen_t{sizeof(sa)};
    assertSuccess(getsockopt(s.native_handle(), SOL_IP, SO_ORIGINAL_DST, &sa, &len));
    return makeEndpoint(helper.buf2address(&sa.sin_addr.s_addr).to_string(), ntohs(sa.sin_port));
  }
  else {
    auto sa  = sockaddr_in6{};
    auto len = socklen_t{sizeof(sa)};
    assertSuccess(getsockopt(s.native_handle(), SOL_IPV6, IP6T_SO_ORIGINAL_DST, &sa, &len));
    return makeEndpoint(helper.buf2address(&sa.sin6_addr.s6_addr).to_string(), ntohs(sa.sin6_port));
  }
}

#endif  // TRANSPARENT_IPTABLES

}  // namespace platform

TransparentIngress::TransparentIngress(vo::Ingress const&, Socket s) : socket_{std::move(s)} {}

Awaitable<size_t> TransparentIngress::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(socket_, buf);
}

Awaitable<void> TransparentIngress::send(ConstBuffer buf) { co_await stream::write(socket_, buf); }

Awaitable<void> TransparentIngress::close() { co_await stream::close(socket_); }

Awaitable<Endpoint> TransparentIngress::read_remote()
{
#ifdef TRANSPARENT_PF
  static auto pf = platform::PfReader{};
  co_return pf.read_remote(socket_.remote_endpoint(), socket_.local_endpoint());
#endif  // TRANSPARENT_PF
#ifdef TRANSPARENT_IPTABLES
  co_return platform::read_remote(socket_);
#endif  // TRANSPARENT_IPTABLES
  fail();
}

Awaitable<void> TransparentIngress::confirm() { co_return; }

Awaitable<void> TransparentIngress::disconnect(sys::error_code const&) { co_return; }

}  // namespace pichi::adapter::tcp
