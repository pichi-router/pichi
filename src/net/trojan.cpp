#include <pichi/common/config.hpp>
// Include config.hpp first
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <iostream>
#include <pichi/common/asserts.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/common/error.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/crypto/hash.hpp>
#include <pichi/net/helper.hpp>
#include <pichi/net/trojan.hpp>
#include <pichi/stream/test.hpp>
#include <pichi/stream/tls.hpp>
#include <pichi/stream/websocket.hpp>
#include <utility>

using namespace std;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace sys = boost::system;
using tcp = asio::ip::tcp;

namespace pichi::net {

static constexpr size_t PWD_LEN = crypto::HashTraits<HashAlgorithm::SHA224>::length * 2;

static size_t copyToBuffer(ConstBuffer<uint8_t> src, MutableBuffer<uint8_t> dst)
{
  if (src.size() == 0 || dst.size() == 0) return 0;
  auto copied = min(src.size(), dst.size());
  copy_n(cbegin(src), copied, begin(dst));
  return copied;
}

string sha224(string_view pwd)
{
  auto bin = vector(PWD_LEN / 2, 0_u8);
  auto sha224 = crypto::Hash<HashAlgorithm::SHA224>{};
  sha224.hash(ConstBuffer<uint8_t>{pwd}, bin);
  return crypto::bin2hex(bin);
}

template <typename Stream>
size_t TrojanIngress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  if (received_.empty()) return readSome(stream_, buf, yield);
  auto copied = copyToBuffer(received_, buf);
  received_.erase(cbegin(received_), cbegin(received_) + copied);
  return copied;
}

template <typename Stream> void TrojanIngress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  write(stream_, buf, yield);
}

template <typename Stream> void TrojanIngress<Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <typename Stream> bool TrojanIngress<Stream>::readable() const
{
  return !received_.empty() || stream_.is_open();
}

template <typename Stream> bool TrojanIngress<Stream>::writable() const
{
  return stream_.is_open();
}

template <typename Stream> void TrojanIngress<Stream>::confirm(Yield) {}

template <typename Stream> Endpoint TrojanIngress<Stream>::readRemote(Yield yield)
{
  try {
    accept(stream_, yield);

    /*
     * To act as a real HTTPS server, like Nginx, the trojan ingress has to read the hashed
     *   password in one time, which is as same as the official trojan.
     *
     * Consuming the password section described in trojan protocol specification:
     *   +-----------------------+---------+-----+
     *   | hex(SHA224(password)) |  CRLF   | CMD |
     *   +-----------------------+---------+-----+
     *   |          56           | X'0D0A' |  1  |
     *   +-----------------------+---------+-----+
     */
    received_.resize(readSome(stream_, received_, yield));
    assertTrue(received_.size() > PWD_LEN + 2, PichiError::BAD_PROTO);

    auto pwd = string{cbegin(received_), cbegin(received_) + PWD_LEN};
    assertTrue(passwords_.find(pwd) != cend(passwords_), PichiError::UNAUTHENTICATED);

    auto first = received_.data() + PWD_LEN;
    assertTrue(*first++ == '\r', PichiError::BAD_PROTO);
    assertTrue(*first++ == '\n', PichiError::BAD_PROTO);
    assertTrue(*first++ == 1_u8, PichiError::BAD_PROTO);

    /*
     * Parsing the trojan request section:
     *   +------+----------+----------+---------+
     *   | ATYP | DST.ADDR | DST.PORT |  CRLF   |
     *   +------+----------+----------+---------+
     *   |  1   | Variable |    2     | X'0D0A' |
     *   +------+----------+----------+---------+
     * Only CONNECT X'01' CMD is supported, and UDP ASSOCIATE X'03' is unimplemented.
     */
    auto left = received_.size() - distance(received_.data(), first);
    auto ret = parseEndpoint([this, yield, &first, &left](auto dst) {
      if (left > 0) {
        auto copied = copyToBuffer({first, left}, dst);
        first += copied;
        dst += copied;
        left -= copied;
      }
      if (dst.size() > 0) {
        read(stream_, dst, yield);
        received_.insert(end(received_), cbegin(dst), cend(dst));
        first = received_.data() + received_.size();
      }
    });

    if (left < 2) {
      received_.resize(received_.size() + 2 - left);
      first = received_.data() + received_.size() - 2;
      read(stream_, {first + left, 2 - left}, yield);
      left = 0;
    }
    else
      left -= 2;
    assertTrue(*first++ == '\r', PichiError::BAD_PROTO);
    assertTrue(*first++ == '\n', PichiError::BAD_PROTO);

    received_.erase(cbegin(received_), cend(received_) - left);
    return ret;
  }
  catch (sys::system_error const& e) {
    if (e.code().category() != PICHI_CATEGORY) throw e;
    cout << "Trojan Error: " << e.what() << endl;
    return remote_;
  }
}

template <typename Stream>
size_t TrojanEgress<Stream>::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(stream_, buf, yield);
}

template <typename Stream> void TrojanEgress<Stream>::send(ConstBuffer<uint8_t> buf, Yield yield)
{
  write(stream_, buf, yield);
}

template <typename Stream> void TrojanEgress<Stream>::close(Yield yield)
{
  pichi::net::close(stream_, yield);
}

template <typename Stream> bool TrojanEgress<Stream>::readable() const { return stream_.is_open(); }

template <typename Stream> bool TrojanEgress<Stream>::writable() const { return stream_.is_open(); }

template <typename Stream>
void TrojanEgress<Stream>::connect(Endpoint const& remote, ResolveResults next, Yield yield)
{
  pichi::net::connect(next, stream_, yield);

  auto buf = array<uint8_t, 512>{};
  auto written = [&](auto p) -> size_t { return distance(buf.data(), p); };

  /*
   * Here's the trojan protocol specification(https://trojan-gfw.github.io/trojan/protocol):
   *   +-----------------------+---------+-----+------+----------+----------+---------+
   *   | hex(SHA224(password)) |  CRLF   | CMD | ATYP | DST.ADDR | DST.PORT |  CRLF   |
   *   +-----------------------+---------+-----+------+----------+----------+---------+
   *   |          56           | X'0D0A' |  1  |  1   | Variable |    2     | X'0D0A' |
   *   +-----------------------+---------+-----+------+----------+----------+---------+
   */

  copy(cbegin(password_), cend(password_), begin(buf));
  auto p = buf.data() + password_.size();
  *p++ = '\r';
  *p++ = '\n';
  *p++ = 1_u8;
  p += serializeEndpoint(remote, {p, 512 - written(p)});
  *p++ = '\r';
  *p++ = '\n';
  write(stream_, {buf.data(), written(p)}, yield);
}

using TlsStream = stream::TlsStream<tcp::socket>;
using WssStream = stream::WsStream<TlsStream>;
template class TrojanIngress<TlsStream>;
template class TrojanEgress<TlsStream>;
template class TrojanIngress<WssStream>;
template class TrojanEgress<WssStream>;

#ifdef BUILD_TEST
template class TrojanIngress<stream::TestStream>;
template class TrojanEgress<stream::TestStream>;
#endif  // BUILD_TEST

}  // namespace pichi::net
