#define BOOST_TEST_MODULE pichi ss test

#include "utils.hpp"
#include <array>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common.hpp>
#include <pichi/config.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/ssaead.hpp>
#include <pichi/net/ssstream.hpp>
#include <pichi/test/socket.hpp>

using namespace std;
using namespace pichi;
using namespace pichi::crypto;
namespace asio = boost::asio;
namespace mpl = boost::mpl;
namespace sys = boost::system;

using test::Socket;
template <CryptoMethod method> using StreamAdapter = net::SSStreamAdapter<method, test::Stream>;
template <CryptoMethod method> using AeadAdapter = net::SSAeadAdapter<method, test::Stream>;

using Adapters = mpl::list<
    StreamAdapter<CryptoMethod::RC4_MD5>, StreamAdapter<CryptoMethod::BF_CFB>,
    StreamAdapter<CryptoMethod::AES_128_CTR>, StreamAdapter<CryptoMethod::AES_192_CTR>,
    StreamAdapter<CryptoMethod::AES_256_CTR>, StreamAdapter<CryptoMethod::AES_128_CFB>,
    StreamAdapter<CryptoMethod::AES_192_CFB>, StreamAdapter<CryptoMethod::AES_256_CFB>,
    StreamAdapter<CryptoMethod::CAMELLIA_128_CFB>, StreamAdapter<CryptoMethod::CAMELLIA_192_CFB>,
    StreamAdapter<CryptoMethod::CAMELLIA_256_CFB>, StreamAdapter<CryptoMethod::CHACHA20_IETF>,
    AeadAdapter<CryptoMethod::AES_128_GCM>, AeadAdapter<CryptoMethod::AES_192_GCM>,
    AeadAdapter<CryptoMethod::AES_256_GCM>, AeadAdapter<CryptoMethod::CHACHA20_IETF_POLY1305>,
    AeadAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305>>;

static asio::detail::Pull* pPull = nullptr;
static asio::detail::Push* pPush = nullptr;
static asio::yield_context yield = {*pPush, *pPull};

template <CryptoMethod method> static constexpr size_t cipherLength(size_t plainLength)
{
  if constexpr (helpers::isStream<method>())
    return plainLength;
  else
    return plainLength + TAG_SIZE<method> * 2 + 2;
}

template <CryptoMethod method>
static size_t encrypt(Encryptor<method>& encryptor, ConstBuffer<uint8_t> plain,
                      MutableBuffer<uint8_t> cipher)
{
  BOOST_REQUIRE_GE(cipher.size(), cipherLength<method>(plain.size()));
  if constexpr (helpers::isStream<method>()) {
    encryptor.encrypt(plain, cipher);
  }
  else {
    auto len = static_cast<uint16_t>(plain.size());
    len = ((len & 0xff) << 8) + ((len >> 8) & 0xff);
    cipher += encryptor.encrypt({reinterpret_cast<uint8_t*>(&len), 2}, cipher);
    encryptor.encrypt(plain, cipher);
  }
  return cipherLength<method>(plain.size());
}

BOOST_AUTO_TEST_SUITE(SS)

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
  fill_n(begin(expect), IV_SIZE<Ingress::METHOD>, 0xff_u8);
  socket.fill(expect);

  auto fact = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  fill_n(begin(fact), IV_SIZE<Ingress::METHOD>, 0_u8);
  BOOST_CHECK_EQUAL(IV_SIZE<Ingress::METHOD>, ingress.readIV(fact, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(readRemote, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Ingress::METHOD>, 0xff_u8);

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  fill_n(begin(iv), IV_SIZE<Ingress::METHOD>, 0xff_u8);

  auto encryptor = Encryptor<Ingress::METHOD>{psk, iv};
  auto expect = net::makeEndpoint("localhost"sv, 443);
  auto plain = array<uint8_t, 1024>{};
  auto cipher = array<uint8_t, 1024>{};

  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};
  socket.fill(iv);
  socket.fill({cipher, encrypt<Ingress::METHOD>(
                           encryptor, {plain, net::serializeEndpoint(expect, plain)}, cipher)});

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
  BOOST_CHECK_EQUAL(0_sz, socket.available());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(disconnect, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};

  ingress.disconnect(PichiError::MISC, yield);
  BOOST_CHECK_EQUAL(0_sz, socket.available());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(recv_Ingress, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Ingress::METHOD>, 0xff_u8);

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  fill_n(begin(iv), IV_SIZE<Ingress::METHOD>, 0xff_u8);

  auto encryptor = Encryptor<Ingress::METHOD>{psk, iv};
  auto expect = array<uint8_t, 1024>{};
  auto cipher = array<uint8_t, cipherLength<Ingress::METHOD>(1024)>{};
  fill_n(begin(expect), expect.size(), 0xff_u8);
  encrypt<Ingress::METHOD>(encryptor, expect, cipher);

  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};
  socket.fill(iv);
  socket.fill(cipher);

  auto fact = array<uint8_t, 1024>{};
  BOOST_CHECK_EQUAL(expect.size(), ingress.recv(fact, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(recv_Ingress_By_Insufficient_Buffer, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Ingress::METHOD>, 0xff_u8);

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  fill_n(begin(iv), IV_SIZE<Ingress::METHOD>, 0xff_u8);

  auto encryptor = Encryptor<Ingress::METHOD>{psk, iv};
  auto expect = array<uint8_t, 1024>{};
  auto cipher = array<uint8_t, cipherLength<Ingress::METHOD>(1024)>{};
  fill_n(begin(expect), expect.size(), 0xff_u8);
  encrypt<Ingress::METHOD>(encryptor, expect, cipher);

  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};
  socket.fill(iv);
  socket.fill(cipher);

  auto fact = array<uint8_t, 1024>{};
  for (auto i = 0_sz; i < expect.size(); ++i)
    BOOST_CHECK_EQUAL(1_sz, ingress.recv({fact.data() + i, 1}, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(send_Ingress, Ingress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Ingress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Ingress::METHOD>, 0xff_u8);

  auto plain = array<uint8_t, 1024>{};
  auto expect = array<uint8_t, cipherLength<Ingress::METHOD>(1024)>{};
  fill_n(begin(plain), plain.size(), 0xee_u8);

  auto socket = Socket{};
  auto ingress = Ingress{psk, socket, true};

  ingress.send(plain, yield);
  BOOST_CHECK_EQUAL(socket.available(), expect.size() + IV_SIZE<Ingress::METHOD>);

  auto iv = array<uint8_t, IV_SIZE<Ingress::METHOD>>{};
  BOOST_CHECK_EQUAL(IV_SIZE<Ingress::METHOD>, socket.flush({iv, IV_SIZE<Ingress::METHOD>}));

  auto fact = array<uint8_t, cipherLength<Ingress::METHOD>(1024)>{};
  auto encryptor = Encryptor<Ingress::METHOD>{psk, iv};
  encrypt<Ingress::METHOD>(encryptor, plain, expect);
  BOOST_CHECK_EQUAL(expect.size(), socket.flush(fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(connect, Egress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Egress::METHOD>>{};
  auto endpoint = net::makeEndpoint("localhost"sv, 443);

  auto socket = Socket{};
  auto egress = Egress{psk, socket};

  egress.connect(endpoint, {}, yield);

  auto iv = array<uint8_t, IV_SIZE<Egress::METHOD>>{};
  BOOST_CHECK_EQUAL(IV_SIZE<Egress::METHOD>, socket.flush(iv));

  auto encryptor = Encryptor<Egress::METHOD>{psk, iv};
  auto plain = array<uint8_t, 1024>{};
  auto expect = array<uint8_t, 1024>{};
  auto len =
      encrypt<Egress::METHOD>(encryptor, {plain, net::serializeEndpoint(endpoint, plain)}, expect);

  auto fact = array<uint8_t, 1024>{};
  BOOST_CHECK_EQUAL(len, socket.flush(fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cbegin(expect) + len, cbegin(fact),
                                cbegin(fact) + len);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(send_Egress, Egress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Egress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Egress::METHOD>, 0xff_u8);

  auto plain = array<uint8_t, 1024>{};
  auto expect = array<uint8_t, cipherLength<Egress::METHOD>(1024)>{};
  fill_n(begin(plain), plain.size(), 0xee_u8);

  auto socket = Socket{};
  auto egress = Egress{psk, socket, true};

  egress.send(plain, yield);
  BOOST_CHECK_EQUAL(socket.available(), expect.size() + IV_SIZE<Egress::METHOD>);

  auto iv = array<uint8_t, IV_SIZE<Egress::METHOD>>{};
  BOOST_CHECK_EQUAL(IV_SIZE<Egress::METHOD>, socket.flush({iv, IV_SIZE<Egress::METHOD>}));
  auto encryptor = Encryptor<Egress::METHOD>{psk, iv};
  encrypt<Egress::METHOD>(encryptor, plain, expect);

  auto fact = array<uint8_t, cipherLength<Egress::METHOD>(1024)>{};
  BOOST_CHECK_EQUAL(expect.size(), socket.flush(fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(recv_Egress, Egress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Egress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Egress::METHOD>, 0xff_u8);

  auto iv = array<uint8_t, IV_SIZE<Egress::METHOD>>{};
  fill_n(begin(iv), IV_SIZE<Egress::METHOD>, 0xff_u8);

  auto expect = array<uint8_t, 1024>{};
  auto cipher = array<uint8_t, cipherLength<Egress::METHOD>(1024)>{};
  auto encryptor = Encryptor<Egress::METHOD>{psk, iv};
  fill_n(begin(expect), expect.size(), 0xff_u8);
  encrypt<Egress::METHOD>(encryptor, expect, cipher);

  auto socket = Socket{};
  auto egress = Egress{psk, socket, true};
  socket.fill(iv);
  socket.fill(cipher);

  auto fact = array<uint8_t, 1024>{};
  BOOST_CHECK_EQUAL(expect.size(), egress.recv(fact, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(recv_Egress_By_Insufficient_Buffer, Egress, Adapters)
{
  auto psk = array<uint8_t, KEY_SIZE<Egress::METHOD>>{};
  fill_n(begin(psk), KEY_SIZE<Egress::METHOD>, 0xff_u8);

  auto iv = array<uint8_t, IV_SIZE<Egress::METHOD>>{};
  fill_n(begin(iv), IV_SIZE<Egress::METHOD>, 0xff_u8);

  auto expect = array<uint8_t, 1024>{};
  auto cipher = array<uint8_t, cipherLength<Egress::METHOD>(1024)>{};
  auto encryptor = Encryptor<Egress::METHOD>{psk, iv};
  fill_n(begin(expect), expect.size(), 0xff_u8);
  encrypt<Egress::METHOD>(encryptor, expect, cipher);

  auto socket = Socket{};
  auto egress = Egress{psk, socket, true};
  socket.fill(iv);
  socket.fill(cipher);

  auto fact = array<uint8_t, 1024>{};
  for (auto i = 0_sz; i < expect.size(); ++i)
    BOOST_CHECK_EQUAL(1_sz, egress.recv({fact.data() + i, 1}, yield));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_SUITE_END()
