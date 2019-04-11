#define BOOST_TEST_MODULE pichi ssstream test

#include "config.h"
#include "utils.hpp"
#include <array>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/ssstream.hpp>
#include <pichi/test/socket.hpp>

using namespace std;
using namespace pichi;
using namespace pichi::crypto;
namespace asio = boost::asio;
namespace mpl = boost::mpl;
namespace sys = boost::system;

using test::Socket;
template <CryptoMethod method> using Adapter = net::SSStreamAdapter<method, test::Stream>;

using Adapters =
    mpl::list<Adapter<CryptoMethod::RC4_MD5>, Adapter<CryptoMethod::BF_CFB>,
              Adapter<CryptoMethod::AES_128_CTR>, Adapter<CryptoMethod::AES_192_CTR>,
              Adapter<CryptoMethod::AES_256_CTR>, Adapter<CryptoMethod::AES_128_CFB>,
              Adapter<CryptoMethod::AES_192_CFB>, Adapter<CryptoMethod::AES_256_CFB>,
              Adapter<CryptoMethod::CAMELLIA_128_CFB>, Adapter<CryptoMethod::CAMELLIA_192_CFB>,
              Adapter<CryptoMethod::CAMELLIA_256_CFB>, Adapter<CryptoMethod::CHACHA20_IETF>>;

static asio::detail::Pull* pPull = nullptr;
static asio::detail::Push* pPush = nullptr;
static asio::yield_context yield = {*pPush, *pPull};

BOOST_AUTO_TEST_SUITE(SSSTREAM)

BOOST_AUTO_TEST_CASE_TEMPLATE(readIV_Duplicated_Read, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};

  socket.fill(iv);
  BOOST_CHECK_EQUAL(IV_SIZE<Ingress::METHOD>, ingress.readIV(iv, yield));

  socket.fill(iv);
  BOOST_CHECK_EXCEPTION(ingress.readIV(iv, yield), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(readIV_Insufficient_Buffer, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  socket.fill(iv);

  BOOST_CHECK_EXCEPTION(ingress.readIV({iv, iv.size() - 1}, yield), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(readIV, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};

  auto expect = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  fill_n(begin(expect), IV_SIZE<Ingress::METHOD>, 0xff);
  socket.fill(expect);

  auto fact = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  fill_n(begin(fact), IV_SIZE<Ingress::METHOD>, 0);
  BOOST_CHECK_EQUAL(IV_SIZE<Ingress::METHOD>, ingress.readIV(fact, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(readRemote, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Ingress::METHOD>, 0xff);

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  fill_n(begin(iv), IV_SIZE<Ingress::METHOD>, 0xff);

  auto encryptor = Encryptor<Ingress::METHOD>{psk, iv};
  auto expect = net::makeEndpoint("localhost"sv, 443);
  auto plain = array<uint8_t, 1024>{};
  auto cipher = array<uint8_t, 1024>{};

  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};
  socket.fill(iv);
  socket.fill({cipher, encryptor.encrypt({plain, net::serializeEndpoint(expect, plain)}, cipher)});

  auto fact = ingress.readRemote(yield);

  BOOST_CHECK(expect.type_ == fact.type_);
  BOOST_CHECK_EQUAL(expect.host_, fact.host_);
  BOOST_CHECK_EQUAL(expect.port_, fact.port_);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(confirm, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};

  ingress.confirm(yield);
  BOOST_CHECK_EQUAL(0, socket.available());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(disconnect, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};

  ingress.disconnect(yield);
  BOOST_CHECK_EQUAL(0, socket.available());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(recv_Ingress, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Ingress::METHOD>, 0xff);

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  fill_n(begin(iv), IV_SIZE<Ingress::METHOD>, 0xff);

  auto expect = array<uint8_t, 1024>{};
  auto cipher = array<uint8_t, 1024>{};
  fill_n(begin(expect), expect.size(), 0xff);
  Encryptor<Ingress::METHOD>{psk, iv}.encrypt(expect, cipher);

  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};
  socket.fill(iv);
  socket.fill(cipher);

  auto fact = array<uint8_t, 1024>{};
  BOOST_CHECK_EQUAL(expect.size(), ingress.recv(fact, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(send_Ingress, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Ingress::METHOD>, 0xff);

  auto plain = array<uint8_t, 1024>{};
  auto expect = array<uint8_t, 1024>{};
  fill_n(begin(plain), plain.size(), 0xee);

  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};

  ingress.send(plain, yield);
  BOOST_CHECK_EQUAL(socket.available(), expect.size() + IV_SIZE<Ingress::METHOD>);

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  BOOST_CHECK_EQUAL(IV_SIZE<Ingress::METHOD>, socket.flush({iv, IV_SIZE<Ingress::METHOD>}));

  auto fact = array<uint8_t, 1024>{};
  Encryptor<Ingress::METHOD>{psk, iv}.encrypt(plain, expect);
  BOOST_CHECK_EQUAL(expect.size(), socket.flush(fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(connect, Egress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Egress::METHOD>>{};
  auto expect = net::makeEndpoint("localhost"sv, 443);

  auto socket = Socket{};
  auto egress = Egress{psk, socket};

  egress.connect(expect, {}, yield);

  auto iv = array<uint8_t, IV_SIZE<Egress::METHOD>>{};
  BOOST_CHECK_EQUAL(IV_SIZE<Egress::METHOD>, socket.flush(iv));
  auto decryptor = Decryptor<Egress::METHOD>{psk};
  decryptor.setIv(iv);

  auto fact = net::parseEndpoint([&](auto plain) {
    BOOST_CHECK_LE(plain.size(), socket.available());
    auto buf = array<uint8_t, 1024>{};
    auto cipher = MutableBuffer<uint8_t>{buf, plain.size()};
    socket.flush(cipher);
    decryptor.decrypt(cipher, plain);
  });

  BOOST_CHECK(expect.type_ == fact.type_);
  BOOST_CHECK_EQUAL(expect.host_, fact.host_);
  BOOST_CHECK_EQUAL(expect.port_, fact.port_);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(send_Egress, Egress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Egress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Egress::METHOD>, 0xff);

  auto plain = array<uint8_t, 1024>{};
  auto expect = array<uint8_t, 1024>{};
  fill_n(begin(plain), plain.size(), 0xee);

  auto socket = Socket{};
  auto egress = Egress{psk, socket, true};

  egress.send(plain, yield);
  BOOST_CHECK_EQUAL(socket.available(), expect.size() + IV_SIZE<Egress::METHOD>);

  auto iv = array<uint8_t, IV_SIZE<Egress::METHOD>>{};
  BOOST_CHECK_EQUAL(IV_SIZE<Egress::METHOD>, socket.flush({iv, IV_SIZE<Egress::METHOD>}));

  auto fact = array<uint8_t, 1024>{};
  Encryptor<Egress::METHOD>{psk, iv}.encrypt(plain, expect);
  BOOST_CHECK_EQUAL(expect.size(), socket.flush(fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(recv_Egress, Egress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Egress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Egress::METHOD>, 0xff);

  auto iv = array<uint8_t, IV_SIZE<Egress::METHOD>>{};
  fill_n(begin(iv), IV_SIZE<Egress::METHOD>, 0xff);

  auto expect = array<uint8_t, 1024>{};
  auto cipher = array<uint8_t, 1024>{};
  fill_n(begin(expect), expect.size(), 0xff);
  Encryptor<Egress::METHOD>{psk, iv}.encrypt(expect, cipher);

  auto socket = Socket{};
  auto egress = Egress{psk, socket, true};
  socket.fill(iv);
  socket.fill(cipher);

  auto fact = array<uint8_t, 1024>{};
  BOOST_CHECK_EQUAL(expect.size(), egress.recv(fact, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_SUITE_END()
