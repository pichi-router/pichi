#define BOOST_TEST_MODULE pichi cryptogram test

#include <algorithm>
#include <array>
#include <boost/test/unit_test.hpp>
#include <botan/hex.h>
#include <pichi/stream/shadowsocks.hpp>
#include <ranges>
#include <unordered_map>

using namespace std::literals;
namespace rngs  = std::ranges;
namespace views = rngs::views;

namespace pichi::unit_test {

using stream::detail::Decryptor;
using stream::detail::Encryptor;

using Buffer = std::array<uint8_t, 1024>;

struct TestData {
  std::string              salt_;
  std::vector<std::string> ciphers_;
};

static auto const PASSWORD = "a flexible rule-based proxy"s;

static auto const PLAINS = std::vector{
    "Any submission t"s,  "o the IETF inten"s,  "ded by the Contr"s, "ibutor for publi"s,
    "cation as all or"s,  " part of an IETF"s,  " Internet-Draft "s, "or RFC and any s"s,
    "tatement made wi"s,  "thin the context"s,  " of an IETF acti"s, "vity is consider"s,
    "ed an \"IETF Cont"s, "ribution\". Such "s, "statements inclu"s, "de oral statemen"s,
    "ts in IETF sessi"s,  "ons, as well as "s,  "written and elec"s, "tronic communica"s,
    "tions made at an"s,  "y time or place,"s,  " which are addre"s, "ssed to"s
};

static auto const TEST_DATA = std::unordered_map<CryptoMethod, TestData>{
    {            CryptoMethod::AES_128_GCM,
     {.salt_ = "000102030405060708090a0b0c0d0e0f"s,
     .ciphers_ =
     {
     "cbd89814e3cf12f4338d2d2f1c7a144ee5582f27defc3469d6c44a0fc848b7d5"s,
     "e770df119903e90d76b7ef6a07dec4449406f67765e4b9b4f180733c209bd313"s,
     "bbae7708769102c52bcf4822166be2596b6a5da861c491718579af4f2b8b83db"s,
     "5a75a2ec91f9605d71606a9766c405e760904cfb8bfc3ea0cbccb48c38d358c2"s,
     "230948be32ff53cd8ba2ebf8bc2fe937a2ee7f52ad5eeab7a9cf5c7d7c49d134"s,
     "a49f4203d2efffae818cde210ab70804e3d470bb01224a63b1e03753c915b5be"s,
     "fdd79fece01a454601abd73fc495d65f2ec593a976ebf975a51272ba64625e51"s,
     "dda2f455d4be6f48eb896055b3fdf7aa07a028730d7500af5312c61eea557e7f"s,
     "83d7cd76eaf913c5b74698cceb0bf7c22e70f0c75eed048e5cb660db15fbca15"s,
     "a9c3442ec6daaeaec60dbb53fe1d0ac795d59fd31201ade09f1b312c69b70854"s,
     "91590b20b7c09ef23d9339daa8e41bb22ab9bb8668b5001da83f134c47d9a5ce"s,
     "4a3b45a0febf3ced92669043d846f75a65dd44ec00044b147e94d476e6acbb9c"s,
     "ae564fcb813340b334001da49f4b3ee84b6b6be4fb675512d3f2044160e9fc80"s,
     "f6fdf01e5207fb66fc9f270770cbaa939516eac3ba5856e227492d32d3b27f9f"s,
     "010f74cc65caeb67eadfe361038e8eeba4d93d2f0d262b0928c9dd335621008d"s,
     "caaa885b41bdeeb1c87c37f4bef506a53c5fb3ba12e84687d90cef6b24d98b63"s,
     "8ee966998371aa310e5c6f372d1f08e29287c8c079d4b875d07a9567f0986037"s,
     "4d61b5485476a339eb4ad12cba65c292a210f39c97d69df54f0c7a8da3e04efc"s,
     "3d08130cedf223e2c3d3b4d7eb94382a9577396f11d47d62580f42c469729fac"s,
     "382c316169fa36e682e7983486690820e8e628515a55cf1143237e2b14eba7f1"s,
     "14bb78a467c1855a1413c6b6da2ad5e6a5288055cd4c45999c113f784c971003"s,
     "b490e5e4925609755167e9d2174e93ef948ce148dbe2f26c6bc4d7ae8a6dbe54"s,
     "fabaf75f6d1c5d618f236bc7b86dca11b49e6e12a3c18772d4b0a9133d2b4cc8"s,
     "c2190b1bae5fba0291f560e21a3bc75a29b9b78925bd36"s,
     }}},
    {            CryptoMethod::AES_192_GCM,
     {.salt_ = "000102030405060708090a0b0c0d0e0f0f0e0d0c0b0a0908"s,
     .ciphers_ =
     {
     "5df59840e115339472c9fc6ff88a76dc75b91c9a6c40c19af7ee02e0952bd84f"s,
     "d77bab72b71023f2c749e417e3dc5065556217195ccd3bcb2abedf568713e14a"s,
     "7540ee1edc44bb278d6eed5910943be2ca3de937b29400b5005fc03c47ce40db"s,
     "76c7fb34c355fd6d79adfaacff797443bd970e52266ee6d8fcb0cb880ed1e713"s,
     "2931628cfdbab1bb9349a8932cb6f10db0e237c9cd17c284b0cdd1689a79fb3b"s,
     "bd5d3d03ae4188c166eb181c4d2f3de40e78eeef100d3cabe4bc8f0cdb18c207"s,
     "7029f7b04fcd0cabfc081120321c4c55b28c0842bd40e94fbccea51da0cff75a"s,
     "617c3e60837a9b0aa19ecbc7e30857bc211286c3bf48c81102b2560cdbeb95b9"s,
     "654c2db91c8239c857d324ef988fabe37095f9455f3e5c37bcd6d2271ed6ff46"s,
     "b02f7a058b81b83359c8238606864dd53d2a5d350e77d32151f884d605395984"s,
     "d0fc8485c67b40e8a386d4ee940d1c38d8836ebfe89af56456fdf220f720a20d"s,
     "b8d9c8e75bb82f6388a6ead8aa3df8be8ed10fd73e637b6f98a1f77f517c3d57"s,
     "0e653795bf69072a26f16fc93aaec0ce9fc1a4cd7188072200e00aed1a999b66"s,
     "953a922402e62a92fd2a9a26a200df630f75b0a38460668e48ea7e30c21e17e7"s,
     "56bc71ca342ac7059bf4b10c514f87b2f92d0a9fc451154225c336eb01edfdff"s,
     "2b6067a1ffce95c21ddb2f507710652937642b96fd7e01d7b4508960381b0bec"s,
     "4e7f9f24f09eb5e69ec11dbd0ff0fefe2d6965aaadb99122e81e81004bcea5a0"s,
     "afb7fe97396746c73b13af7c338ad6e2cb32c12146460575bf90caa28ab3f890"s,
     "7f6e46ec8adaab322712e64414070ab12896d43be271c5f91d7822c8f2332460"s,
     "c9c9dcd6d7cab11726f8e6d945a4a2690d3d0e3322951e6f213fd98faef69bc6"s,
     "5ba16435fb5eb5370e3bf495862d978db85891ac34d0cd82664264ee304f8b4d"s,
     "7a22041dd76ab5aa2ac5ba45ad387faa48f69e16315101ffda6e76941fc6d1f0"s,
     "7b92479c8fc37424f1d89eb616e3f02a35dd3660586d38ee21b2ae5b061fa335"s,
     "3690ef8065d0fd204aaa9e6db5425feb5da337cc099e45"s,
     }}},
    {            CryptoMethod::AES_256_GCM,
     {.salt_ = "000102030405060708090a0b0c0d0e0f0f0e0d0c0b0a09080706050403020100"s,
     .ciphers_ =
     {
     "ac0e3fb3017afafba4fd4bfff80d48fe7ee3323f80e056d74dc456bdeb56c07b"s,
     "f3689a210674e2ffecdcb88a5047c51dd3cc66c6447d1c9a96915faedc51046e"s,
     "633c6888fac2d8693579d0d7d3f9e429ae95ffdeba5dfc3bd69fbd11faff9fcd"s,
     "6a977ba14daec82ae35ad6597fd7d9ce4cc199edc57d502e738caeb47757a660"s,
     "6bfc609396c3fef6d64adc6c408a0c21410d8fe2908dab7af0452b546e3c1e2e"s,
     "fd63b5fa7e800220d278f491324dd09bd71994fcd2e023060c3231659c09af6d"s,
     "ebfa59ae13e0445ad52badb0f65782e9247b960d294a02a4901d67049e2015e6"s,
     "d74df1f01c0ace386834f11b1be73d9453a6e7c028ae8a70a92d838d4bddc044"s,
     "d5b56c607f309cfd8e56de8f1992f88fd6db09b7f3a94c4fdea467bff42217ed"s,
     "2e94b93976757901fd9430676924b8ea03b0706878fcb8031edee09e4a0a4be3"s,
     "ffc42ca91964e4efdde79f980f42a7c665ea9724b93e587f48e733730603c231"s,
     "e375e1e217091eb9b3ceae96b7be64355a226e4988850a31d61c742ee706b7fe"s,
     "86a51a65b2e4af169a7a375a79b64714cd7f605d8420e44f06680c18e5a0381c"s,
     "f6d28c32824f05b7f7e7b02e6b0af4334b1417f2deaba28ee10e9aed2eea9c0a"s,
     "4c899d5393db3195149616467308ed6967b78b18ee745d8b6947d40006f07755"s,
     "9f970ea57d2fd6a11dd17815b23ed33ab06d35c4ae88b2144ec2d5754e5f72a5"s,
     "24117c50bb75e06c6b1b9953ce32430b0ac65e402f6f965d2bb9e21ef4c0650b"s,
     "5c88e869f69296cc2899f8d5ad6b58074c80fe3d270f2687a9b036ad55cdd480"s,
     "e5b2289ba9b2be8a5675576631cc95ae41a5edf270b61aa839ac4e73c4b3c2aa"s,
     "011d43ec0a833b42d09b951c20e133e6cfb6029250556117221dcc0a27000606"s,
     "0c079b1cb9159441bbea930703d8b411237e500ebd2330838a007ad9c1056c38"s,
     "747088b002a7fe48b3f3ce75d3ff746a74f32b6edaaa31524d3174e891f5b3ca"s,
     "d802deb23612f72968a06c1f593e2773c07c169f0632d7f8e5763b7209c23d9d"s,
     "2a1ac6103f35ce6d9ffbb972dd02c4cb9c5229f43bd559"s,
     }}},
    { CryptoMethod::CHACHA20_IETF_POLY1305,
     {.salt_ = "000102030405060708090a0b0c0d0e0f0f0e0d0c0b0a09080706050403020100"s,
     .ciphers_ =
     {
     "a6f26ba87a0756c1224833947023b15494e6b1682aa3dde920641f4f7e0fc999"s,
     "ec195691d137d57307750b739ae4fbee887af2ee1a561ef6b363fd04ed52ed5a"s,
     "79e516fe1504f245fac8f43ebd9d177e757d7f70623e155aa98911b868f9e2e2"s,
     "f906cf71c62a318efdfa53e204ac0bd49744d6c8f6aa4b2b8b7b203c21d55ee4"s,
     "d38d71e5a02c844a0f2654c8d6a7166aa930da0b04624fba9cadc2ad201a32c7"s,
     "5dafb0fb58390629141275dc5bfece02c036490db5e623751dacb4bc37aca441"s,
     "f870d332ee95b9d9c6e8457d40d86dccb29e1adf817dd1952b340a78f20b5878"s,
     "a3971838f77713ef1876e35f2f04a0bbe5cccb0c15609872d4665c2a68f4d6d5"s,
     "cde4bbbdf208a41383afb1cddaf19882424ac57d34cb7a92a37945bb33e31465"s,
     "579819c86168110cd5008b5d675f1572081716690f6cd7dfbc9e42f4b8487356"s,
     "700dc2b45641ebaaa664df9083a397967403c041770085c06ef7c1d03f095e44"s,
     "619179b3999574fb3fca55d581e35580b0d3e878ad02ef0f03bde799a695b124"s,
     "86a02b617b4b2d1449bcb6e4f82aefeac159fb94f2a6e77f901e655c189c107e"s,
     "ffcbe847df46a2b241143bae00ecd369af32cb46b6102087d3440e2960bd9268"s,
     "029d29e3116f81acf669bba546e7f12bb13ab98bc85b3d5972412c4cd37e4446"s,
     "5748b7859371b99d9216cbfbfa185f3030c9b923e64bed86c07bf524db51483d"s,
     "4ef4e0350b8022b791b850125a88caa6cce436148abf2a83f6007d394feddd5c"s,
     "ea3e61b3c83a0aa9615583e46f86551b8ac540205576a0e7db257546babf4a22"s,
     "49846eb0f0992b2b4a701323a8bcb31887a0f0f538479406ebbfa14d9099321b"s,
     "57b11f052a1ed65b158cfc1299144f795f3c6ea6d05ac81cfa07663812baf1ed"s,
     "8e59ed9aa14defc8c288329b03b431cad8e7a8b85e9b0475ab7e49af0405a305"s,
     "210a519bc4be3b1af280c5f2d5bff6f844638eee2cd9143d20a0cd35aade713c"s,
     "5253abc34c8bc97ca36096cb6cf80c77d18f36c66b78e2fdd5799c6d22a98d8d"s,
     "2284052ceac971cec204019f9adfda2ad44ab0c323c5ac"s,
     }}},
    {CryptoMethod::XCHACHA20_IETF_POLY1305,
     {.salt_ = "000102030405060708090a0b0c0d0e0f0f0e0d0c0b0a09080706050403020100"s,
     .ciphers_ =
     {
     "3f44beac0d3d42b71bef662f52173755a78a53656f703d4049676cae189d78f3"s,
     "c9cf8af3e08185048bc17fcabc9bb05f0a66634a295e649dae2223c4dd5a9cc3"s,
     "9ff38ac231666acf79070c013cd68add359f087bb13c9ef2177635f582e800bd"s,
     "efb51a219e8faad4d018774651f272299456da6a2eac4725b473f79eb8b02756"s,
     "f79fc3c18da915d0a63085786802cde0c0b6300a3bec777a9e2d13c033e461d0"s,
     "296d57253dfc0d13b73bfd810ac4132930d52bad10e6d24e1ae25ba6f00c46a0"s,
     "0c8fde0134cd0bec56c9f7d0fbae1f6bfeb71a51249d99bdaa323641bf3427c8"s,
     "f4238bbc35a0361ea7407f23b223537df068f32efa1a7e479e8132d5675d3626"s,
     "6e8307eeeb5ae1954bdc388cd3a56cefe02df74323c74cf2188f38e83a6ee75e"s,
     "8943b8fa43400dee67d3c1d302f54beebdefa239984fc9288f193055e075fb51"s,
     "d40daf8534644213819b907760974455f829deceee4b6b291282173d0748dd28"s,
     "5ff175b09a4de25c2e06cefb031fde8fe4efec58630b18b3a70f7a5a38a4f822"s,
     "75a5da9ba7b872db3371997cadbcf86857117a1a29839cbebe0d8494407cddf1"s,
     "a7c66bded695d2e553a0a0e2a8665f0b37355c10adab36b5c97eaa6c93e6845d"s,
     "fcca914b988796920d90e136efe28f143d5d1db3148ea72edd1ef451bd4b2cee"s,
     "b6591972a53debfc09953a3ea29153aaa4471fe04147ecb1ee09b51ba6de62ff"s,
     "d55df17fa851c1db2b3fc35f5062b61d4061ff395b4fbbffc52f12290d0b644a"s,
     "00c5fac5741216bb04c5c9830c02a90306ccd7b8661b7c894d5cafb8fb4988e0"s,
     "a7a820bb2320880eca3572ca3c5d4d94a5a6027f4c32ed50c30906637a06b424"s,
     "15551e29813f2ca3ad22bc041533f150f6949b7567f91e4e1b4dde8b358d2b2d"s,
     "52f52a533462d814c91140c787426cb61e2a3ba4a204564f843f8ebc2ee3d2e9"s,
     "811243d8b64c90410b58de6a5861f9825c45a1495d4fea57897f040dcb956b0b"s,
     "6621c0430c43fb1d5dca9e09edb62480fc45c9da699383e0b084fd882a4a8d16"s,
     "efb71e1a892807cfb117b6664effe71b7ff39a8ff41a43"s,
     }}},
};

BOOST_AUTO_TEST_SUITE(Cryptogram)

BOOST_AUTO_TEST_CASE(Encryption)
{
  rngs::for_each(TEST_DATA | views::keys, [](auto&& method) {
    auto encryptor = Encryptor{method, PASSWORD};
    auto decryptor = Decryptor{method};
    auto salt      = decryptor.salt();
    rngs::copy(encryptor.salt(), rngs::begin(salt));
    decryptor.set_psk(PASSWORD);
    BOOST_CHECK(rngs::equal(PLAINS, PLAINS | views::transform([&](auto&& plain) {
                                      auto c = Buffer{};
                                      auto n = encryptor.process(plain, c);
                                      auto p = std::string(rngs::size(plain), 0);

                                      decryptor.process(ConstBuffer{c, n}, p);
                                      return p;
                                    })));
  });
}

BOOST_AUTO_TEST_CASE(Encryption_With_Empty_Password)
{
  rngs::for_each(TEST_DATA | views::keys, [](auto&& method) {
    auto password  = ""s;
    auto encryptor = Encryptor{method, password};
    auto decryptor = Decryptor{method};
    auto salt      = decryptor.salt();
    rngs::copy(encryptor.salt(), rngs::begin(salt));
    decryptor.set_psk(password);
    BOOST_CHECK(rngs::equal(PLAINS, PLAINS | views::transform([&](auto&& plain) {
                                      auto c = Buffer{};
                                      auto n = encryptor.process(plain, c);
                                      auto p = std::string(rngs::size(plain), 0);

                                      decryptor.process(ConstBuffer{c, n}, p);
                                      return p;
                                    })));
  });
}

BOOST_AUTO_TEST_CASE(Decryption)
{
  rngs::for_each(TEST_DATA, [](auto&& item) {
    auto&& [m, td] = item;
    auto decryptor = Decryptor{m};

    Botan::hex_decode(decryptor.salt(), td.salt_);
    decryptor.set_psk(PASSWORD);

    auto results = td.ciphers_ | views::transform([&](auto&& cipher) {
                     auto c = Buffer{};
                     auto d = Buffer{};
                     auto p =
                         ConstBuffer{d, decryptor.process({c, Botan::hex_decode(c, cipher)}, d)};
                     return std::string{rngs::begin(p), rngs::end(p)};
                   });

    BOOST_CHECK(rngs::equal(PLAINS, results));
  });
}

BOOST_AUTO_TEST_CASE(Decryption_Invalid_Tag)
{
  rngs::for_each(TEST_DATA, [](auto&& item) {
    auto&& [m, td] = item;
    auto decryptor = Decryptor{m};

    Botan::hex_decode(decryptor.salt(), td.salt_);
    decryptor.set_psk(PASSWORD);

    auto c = Buffer{};
    auto d = Buffer{};
    auto n = Botan::hex_decode(c, td.ciphers_.front());

    // Redact the tag of cipher
    c[n - 1] ^= 0xff;

    BOOST_CHECK_THROW(decryptor.process({c, n}, d), Botan::Invalid_Authentication_Tag);
  });
}

BOOST_AUTO_TEST_CASE(Decryption_Invalid_Cipher)
{
  rngs::for_each(TEST_DATA, [](auto&& item) {
    auto&& [m, td] = item;
    auto decryptor = Decryptor{m};

    Botan::hex_decode(decryptor.salt(), td.salt_);
    decryptor.set_psk(PASSWORD);

    auto c = Buffer{};
    auto d = Buffer{};
    auto n = Botan::hex_decode(c, td.ciphers_.front());

    // Redact the cipher
    c[0] ^= 0xff;

    BOOST_CHECK_THROW(decryptor.process({c, n}, d), Botan::Invalid_Authentication_Tag);
  });
}

BOOST_AUTO_TEST_CASE(Decryption_Invalid_Salt)
{
  rngs::for_each(TEST_DATA, [](auto&& item) {
    auto&& [m, td] = item;
    auto decryptor = Decryptor{m};

    // Salt is not set
    decryptor.set_psk(PASSWORD);

    auto c = Buffer{};
    auto d = Buffer{};

    BOOST_CHECK_THROW(
        decryptor.process({c, Botan::hex_decode(c, td.ciphers_.front())}, d),
        Botan::Invalid_Authentication_Tag
    );
  });
}

BOOST_AUTO_TEST_CASE(Decryption_Without_PSK)
{
  rngs::for_each(TEST_DATA, [](auto&& item) {
    auto&& [m, td] = item;
    auto decryptor = Decryptor{m};

    // Without PSK

    auto c = Buffer{};
    auto d = Buffer{};

    BOOST_CHECK_THROW(
        decryptor.process({c, Botan::hex_decode(c, td.ciphers_.front())}, d),
        Botan::Key_Not_Set
    );
  });
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
