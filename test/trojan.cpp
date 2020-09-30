#define BOOST_TEST_MODULE pichi trojan test

#include "utils.hpp"
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <boost/test/unit_test.hpp>
#include <initializer_list>
#include <memory>
#include <pichi/common/literals.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/stream.hpp>
#include <pichi/net/trojan.hpp>
#include <vector>

using namespace std;

namespace pichi::unit_test {

namespace asio = boost::asio;
namespace sys = boost::system;

using Socket = net::TestSocket;
using Ingress = net::TrojanIngress<net::TestStream>;
using Egress = net::TrojanEgress<net::TestStream>;

static auto const PASSWORDS = vector{"pichi"s};
static auto const HASHED_PASSWORDS = []() {
  auto ret = vector<string>{};
  transform(cbegin(PASSWORDS), cend(PASSWORDS), back_inserter(ret), &net::sha224);
  return ret;
}();
static auto const PWD_LENGTH = HASHED_PASSWORDS.front().size();

static auto FALIED_EP = makeEndpoint("localhost", "80");
static auto CORRECT_EP = makeEndpoint("127.0.0.1", "80");
static auto CORRECT_DATA = []() {
  auto ret = vector<uint8_t>{cbegin(HASHED_PASSWORDS.front()), cend(HASHED_PASSWORDS.front())};
  auto ep = array<uint8_t, 512>{};
  ret.push_back('\r');
  ret.push_back('\n');
  ret.push_back(1_u8);
  ret.insert(end(ret), cbegin(ep), cbegin(ep) + serializeEndpoint(CORRECT_EP, ep));
  ret.push_back('\r');
  ret.push_back('\n');
  return ret;
}();

static auto createIngress(Socket& socket)
{
  return make_unique<Ingress>(FALIED_EP, cbegin(PASSWORDS), cend(PASSWORDS), socket, true);
}

static void verifyFailedRequest(Ingress& ingress, ConstBuffer<uint8_t> expect)
{
  BOOST_CHECK(ingress.readable());

  auto fact = vector(expect.size() + 1, 0_u8);
  fact.resize(ingress.recv(fact, gYield));
  BOOST_CHECK_EQUAL(fact.size(), expect.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(fact), cend(fact), cbegin(expect), cend(expect));
}

BOOST_AUTO_TEST_SUITE(TROJAN)

BOOST_AUTO_TEST_CASE(readRemote_Correct_Stream)
{
  auto socket = Socket{};
  auto ingress = createIngress(socket);
  socket.fill(CORRECT_DATA);
  BOOST_CHECK(CORRECT_EP == ingress->readRemote(gYield));
  BOOST_CHECK(ingress->readable());
  BOOST_CHECK_EXCEPTION(ingress->recv({}, gYield), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(readRemote_Simulate_HTTP_Server)
{
  for (auto i = 1_sz; i < PWD_LENGTH + 3; ++i) {
    auto socket = Socket{};
    auto ingress = createIngress(socket);

    auto data = ConstBuffer<uint8_t>{CORRECT_DATA};
    socket.fill(ConstBuffer<uint8_t>{data, i});

    data += i;
    socket.fill(data);

    BOOST_CHECK(FALIED_EP == ingress->readRemote(gYield));
    verifyFailedRequest(*ingress, {CORRECT_DATA, i});
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Unauthenticated)
{
  auto data = CORRECT_DATA;
  fill_n(begin(data), PWD_LENGTH, 0_u8);
  auto socket = Socket{};
  auto ingress = createIngress(socket);

  socket.fill(data);

  BOOST_CHECK(FALIED_EP == ingress->readRemote(gYield));
  verifyFailedRequest(*ingress, data);
}

BOOST_AUTO_TEST_CASE(readRemote_Invalid_Char_For_The_First_CR)
{
  auto data = CORRECT_DATA;
  for (auto i = 0; i < 0x100; ++i) {
    auto c = static_cast<uint8_t>(i);
    if (c == '\r') continue;
    data[PWD_LENGTH] = c;
    auto socket = Socket{};
    auto ingress = createIngress(socket);

    socket.fill(data);

    BOOST_CHECK(FALIED_EP == ingress->readRemote(gYield));
    verifyFailedRequest(*ingress, data);
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Invalid_Char_For_The_Second_CR)
{
  auto data = CORRECT_DATA;
  for (auto i = 0; i < 0x100; ++i) {
    auto c = static_cast<uint8_t>(i);
    if (c == '\r') continue;
    data[data.size() - 2] = c;
    auto socket = Socket{};
    auto ingress = createIngress(socket);

    socket.fill(data);

    BOOST_CHECK(FALIED_EP == ingress->readRemote(gYield));
    verifyFailedRequest(*ingress, data);
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Invalid_Char_For_The_First_LF)
{
  auto data = CORRECT_DATA;
  for (auto i = 0; i < 0x100; ++i) {
    auto c = static_cast<uint8_t>(i);
    if (c == '\n') continue;
    data[PWD_LENGTH + 1] = c;
    auto socket = Socket{};
    auto ingress = createIngress(socket);

    socket.fill(data);

    BOOST_CHECK(FALIED_EP == ingress->readRemote(gYield));
    verifyFailedRequest(*ingress, data);
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Invalid_Char_For_The_Second_LF)
{
  auto data = CORRECT_DATA;
  for (auto i = 0; i < 0x100; ++i) {
    auto c = static_cast<uint8_t>(i);
    if (c == '\n') continue;
    data[data.size() - 1] = c;
    auto socket = Socket{};
    auto ingress = createIngress(socket);

    socket.fill(data);

    BOOST_CHECK(FALIED_EP == ingress->readRemote(gYield));
    verifyFailedRequest(*ingress, data);
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Invalid_CMD)
{
  auto data = CORRECT_DATA;
  for (auto i = 0; i < 0x100; ++i) {
    auto c = static_cast<uint8_t>(i);
    if (c == 1_u8) continue;
    data[PWD_LENGTH + 2] = c;
    auto socket = Socket{};
    auto ingress = createIngress(socket);

    socket.fill(data);

    BOOST_CHECK(FALIED_EP == ingress->readRemote(gYield));
    verifyFailedRequest(*ingress, data);
  }
}

BOOST_AUTO_TEST_CASE(readRemote_Stream_Separated_From_Trojan_Request)
{
  auto data = CORRECT_DATA;
  for (auto i = PWD_LENGTH + 3; i < data.size(); ++i) {
    auto socket = Socket{};
    auto ingress = createIngress(socket);
    auto buf = ConstBuffer<uint8_t>{data};
    socket.fill({buf, i});
    buf += i;
    socket.fill(buf);

    BOOST_CHECK(CORRECT_EP == ingress->readRemote(gYield));
  }
}

BOOST_AUTO_TEST_CASE(connect_Correct_Request)
{
  auto received = vector(CORRECT_DATA.size(), 0_u8);
  auto socket = Socket{};
  auto egress = make_unique<Egress>(PASSWORDS.front(), socket);

  egress->connect(CORRECT_EP, {}, gYield);

  BOOST_CHECK_EQUAL(CORRECT_DATA.size(), socket.available());
  socket.flush(received);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(received), cend(received), cbegin(CORRECT_DATA),
                                cend(CORRECT_DATA));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
