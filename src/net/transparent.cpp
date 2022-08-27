#include <pichi/common/config.hpp>
// Include config.hpp first
#include <algorithm>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/net/helper.hpp>
#include <pichi/net/transparent.hpp>

#ifdef TRANSPARENT_PF

extern "C" {
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <net/pfvar.h>
}

static decltype(auto) PF_DEVICE = "/dev/pf";

#endif  // TRANSPARENT_PF

#ifdef TRANSPARENT_IPTABLES

extern "C" {
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6/ip6_tables.h>
#include <netinet/in.h>
#include <sys/socket.h>
}

#endif  // TRANSPARENT_IPTABLES

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;
using ip::tcp;

namespace pichi::net {

namespace platform {

class AddressHelper {
private:
  static bool isV6(ip::address const& addr)
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
    using Bytes = typename Address::bytes_type;
    copy_n(cbegin(src.to_bytes()), sizeof(Bytes), reinterpret_cast<uint8_t*>(dst));
  }

  template <typename Address, typename T> Address b2a(T* src)
  {
    using Bytes = typename Address::bytes_type;
    auto dst = Bytes{};
    copy_n(reinterpret_cast<uint8_t*>(src), sizeof(Bytes), begin(dst));
    return Address{dst};
  }

public:
  AddressHelper(ip::address const& addr) : isV6_{AddressHelper::isV6(addr)} {}

  template <typename T> void address2Buf(ip::address const& src, T* dst)
  {
    assertTrue(isV6_ == AddressHelper::isV6(src));
    if (isV6_)
      a2b(src.to_v6(), dst);
    else
      a2b(src.to_v4(), dst);
  }

  template <typename T> ip::address buf2Address(T* src)
  {
    if (isV6_)
      return b2a<ip::address_v6>(src);
    else
      return b2a<ip::address_v4>(src);
  }

  int family() { return isV6_ ? AF_INET6 : AF_INET; }

private:
  bool isV6_;
};

#ifdef TRANSPARENT_PF

class PfReader {
public:
  PfReader() : pf_{open(PF_DEVICE, O_RDONLY)}
  {
    assertSyscallSuccess(pf_);

#ifndef PRIVATE
    // DIOCGETSTATUS operation will be failed on Darwin-XNU, so skip checking
    auto status = pf_status{};
    assertSyscallSuccess(ioctl(pf_, DIOCGETSTATUS, &status));
    assertTrue(status.running, "pf is disabled");
#endif  // PRIVATE
  }

  ~PfReader() { ::close(pf_); }

  Endpoint readRemote(tcp::endpoint const& src, tcp::endpoint const& dst)
  {
    auto helper = AddressHelper{src.address()};
    auto pnl = pfioc_natlook{};
    fill_n(reinterpret_cast<uint8_t*>(&pnl), sizeof(pnl), 0_u8);
    pnl.direction = PF_OUT;
    pnl.af = helper.family();
    pnl.proto = IPPROTO_TCP;
    helper.address2Buf(src.address(), pnl.saddr.addr8);
    helper.address2Buf(dst.address(), pnl.daddr.addr8);

#ifdef PRIVATE
    pnl.sxport.port = htons(src.port());
    pnl.dxport.port = htons(dst.port());
#else   // PRIVATE
    pnl.sport = htons(src.port());
    pnl.dport = htons(dst.port());
#endif  // PRIVATE

    assertSyscallSuccess(ioctl(pf_, DIOCNATLOOK, &pnl));

    return makeEndpoint(helper.buf2Address(pnl.rdaddr.addr8).to_string(),
#ifdef PRIVATE
                        ntohs(pnl.rdxport.port)
#else   // PRIVATE
                        ntohs(pnl.rdport)
#endif  // PRIVATE
    );
  }

private:
  int pf_;
};

#endif  // TRANSPARENT_PF

#ifdef TRANSPARENT_IPTABLES

static Endpoint readRemote(tcp::socket& s)
{
  auto helper = AddressHelper{s.remote_endpoint().address()};
  if (helper.family() == AF_INET) {
    auto sa = sockaddr_in{};
    auto len = socklen_t{sizeof(sa)};
    assertSyscallSuccess(getsockopt(s.native_handle(), SOL_IP, SO_ORIGINAL_DST, &sa, &len));
    return makeEndpoint(helper.buf2Address(&sa.sin_addr.s_addr).to_string(), ntohs(sa.sin_port));
  }
  else {
    auto sa = sockaddr_in6{};
    auto len = socklen_t{sizeof(sa)};
    assertSyscallSuccess(getsockopt(s.native_handle(), SOL_IPV6, IP6T_SO_ORIGINAL_DST, &sa, &len));
    return makeEndpoint(helper.buf2Address(&sa.sin6_addr.s6_addr).to_string(), ntohs(sa.sin6_port));
  }
}

#endif  // TRANSPARENT_IPTABLES

}  // namespace platform

template <typename Socket>
size_t TransparentIngress<Socket>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return pichi::net::readSome(socket_, buf, yield);
}

template <typename Socket>
void TransparentIngress<Socket>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  pichi::net::write(socket_, buf, yield);
}

template <typename Socket> void TransparentIngress<Socket>::close(Yield yield)
{
  pichi::net::close(socket_, yield);
}

template <typename Socket> bool TransparentIngress<Socket>::readable() const
{
  return socket_.is_open();
}

template <typename Socket> bool TransparentIngress<Socket>::writable() const
{
  return socket_.is_open();
}

template <typename Socket> void TransparentIngress<Socket>::confirm(Yield) {}

template <typename Socket> Endpoint TransparentIngress<Socket>::readRemote(Yield)
{
#ifdef TRANSPARENT_PF
  static auto pfReader = platform::PfReader{};
  return pfReader.readRemote(socket_.remote_endpoint(), socket_.local_endpoint());
#endif  // TRANSPARENT_PF
#ifdef TRANSPARENT_IPTABLES
  return platform::readRemote(socket_);
#endif  // TRANSPARENT_IPTABLES
  fail();
}

template class TransparentIngress<boost::asio::ip::tcp::socket>;

}  // namespace pichi::net
