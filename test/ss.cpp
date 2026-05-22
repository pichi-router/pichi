#define BOOST_TEST_MODULE pichi ss test

#include "pichi/common/config.hpp"
#include "utils.hpp"
#include <algorithm>
#include <botan/exceptn.h>
#include <botan/hex.h>
#include <pichi/adapter/tcp/shadowsocks.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/service/sentry.hpp>
#include <pichi/stream/shadowsocks.hpp>
#include <pichi/stream/test.hpp>
#include <utility>

using namespace std::literals;
namespace asio  = boost::asio;
namespace rngs  = std::ranges;
namespace views = rngs::views;

namespace pichi::unit_test {

using Stream  = stream::Shadowsocks<TestSocket>;
using Egress  = adapter::tcp::Shadowsocks<TestSocket>;
using Ingress = adapter::tcp::Shadowsocks<TestSocket>;

using stream::detail::Decryptor;
using stream::detail::Encryptor;

using Buffer = std::array<uint8_t, 1024>;

struct TestData {
  std::string              salt_;
  std::vector<std::string> ciphers_;
};

static auto const PASSWORD = "a flexible rule-based proxy"s;
static auto const ENDPOINT = makeEndpoint("localhost", 443);

static Awaitable<void> send_salt(IOExecutor const& ex, CryptoMethod m, size_t s, uint8_t c)
{
  auto salt   = std::vector<uint8_t>(s, c);
  auto client = TestSocket{ex};
  auto stream = Stream{m, PASSWORD, client.peer()};

  co_await stream::write(client, salt);

  co_await stream.async_accept(asio::use_awaitable);
}

static auto const KEY_SIZE = std::unordered_map<CryptoMethod, size_t>{
    {            CryptoMethod::AES_128_GCM, 16_sz},
    {            CryptoMethod::AES_192_GCM, 24_sz},
    {            CryptoMethod::AES_256_GCM, 32_sz},
    { CryptoMethod::CHACHA20_IETF_POLY1305, 32_sz},
    {CryptoMethod::XCHACHA20_IETF_POLY1305, 32_sz},
};

static auto const DATA = std::unordered_map<CryptoMethod, std::string>{
    {            CryptoMethod::AES_128_GCM,
     "E95FF2D7EAB2552EC8E4C2D33C727FC578CC5457335FA7566051A178E9AFE2BEECD071C9B5A3DA726ED3E9DE38F50"
     "8"},
    {            CryptoMethod::AES_192_GCM,
     "D2B1AA865329B888117231A164414DF424FC143C9B4FB240A74D94AECEA54F25FFD9E6161432EA90ED71EF88711A7"
     "3"},
    {            CryptoMethod::AES_256_GCM,
     "E93213591C3387CFCAA0F38D9C9CF69F9BD8E82446F8DD54A5BDBDC5B58E7946F8AEA3AC6BD51C5467D0A7BFEC9DA"
     "0"},
    { CryptoMethod::CHACHA20_IETF_POLY1305,
     "5CCDE4C3DB7B1F4EFD857CC55B7CD166B1F9D1281B055C7866F08D4C04B0CAF1E533D8FB49A1938A7617ED5CCF552"
     "1"},
    {CryptoMethod::XCHACHA20_IETF_POLY1305,
     "7A780231D50A0A1C53E6C9BB9FBD887B7839B3574E5CE769321BF21805E4EC7A9092235C968649FB6ABF45488F5F2"
     "B"},
};

static auto SALT = std::vector<uint8_t>(32_sz, 0x0f_u8);

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
     {.salt_ = "000102030405060708090A0B0C0D0E0F"s,
     .ciphers_ =
     {
     "CBD89814E3CF12F4338D2D2F1C7A144EE5582F27DEFC3469D6C44A0FC848B7D5"s,
     "E770DF119903E90D76B7EF6A07DEC4449406F67765E4B9B4F180733C209BD313"s,
     "BBAE7708769102C52BCF4822166BE2596B6A5DA861C491718579AF4F2B8B83DB"s,
     "5A75A2EC91F9605D71606A9766C405E760904CFB8BFC3EA0CBCCB48C38D358C2"s,
     "230948BE32FF53CD8BA2EBF8BC2FE937A2EE7F52AD5EEAB7A9CF5C7D7C49D134"s,
     "A49F4203D2EFFFAE818CDE210AB70804E3D470BB01224A63B1E03753C915B5BE"s,
     "FDD79FECE01A454601ABD73FC495D65F2EC593A976EBF975A51272BA64625E51"s,
     "DDA2F455D4BE6F48EB896055B3FDF7AA07A028730D7500AF5312C61EEA557E7F"s,
     "83D7CD76EAF913C5B74698CCEB0BF7C22E70F0C75EED048E5CB660DB15FBCA15"s,
     "A9C3442EC6DAAEAEC60DBB53FE1D0AC795D59FD31201ADE09F1B312C69B70854"s,
     "91590B20B7C09EF23D9339DAA8E41BB22AB9BB8668B5001DA83F134C47D9A5CE"s,
     "4A3B45A0FEBF3CED92669043D846F75A65DD44EC00044B147E94D476E6ACBB9C"s,
     "AE564FCB813340B334001DA49F4B3EE84B6B6BE4FB675512D3F2044160E9FC80"s,
     "F6FDF01E5207FB66FC9F270770CBAA939516EAC3BA5856E227492D32D3B27F9F"s,
     "010F74CC65CAEB67EADFE361038E8EEBA4D93D2F0D262B0928C9DD335621008D"s,
     "CAAA885B41BDEEB1C87C37F4BEF506A53C5FB3BA12E84687D90CEF6B24D98B63"s,
     "8EE966998371AA310E5C6F372D1F08E29287C8C079D4B875D07A9567F0986037"s,
     "4D61B5485476A339EB4AD12CBA65C292A210F39C97D69DF54F0C7A8DA3E04EFC"s,
     "3D08130CEDF223E2C3D3B4D7EB94382A9577396F11D47D62580F42C469729FAC"s,
     "382C316169FA36E682E7983486690820E8E628515A55CF1143237E2B14EBA7F1"s,
     "14BB78A467C1855A1413C6B6DA2AD5E6A5288055CD4C45999C113F784C971003"s,
     "B490E5E4925609755167E9D2174E93EF948CE148DBE2F26C6BC4D7AE8A6DBE54"s,
     "FABAF75F6D1C5D618F236BC7B86DCA11B49E6E12A3C18772D4B0A9133D2B4CC8"s,
     "C2190B1BAE5FBA0291F560E21A3BC75A29B9B78925BD36"s,
     }}},
    {            CryptoMethod::AES_192_GCM,
     {.salt_ = "000102030405060708090A0B0C0D0E0F0F0E0D0C0B0A0908"s,
     .ciphers_ =
     {
     "5DF59840E115339472C9FC6FF88A76DC75B91C9A6C40C19AF7EE02E0952BD84F"s,
     "D77BAB72B71023F2C749E417E3DC5065556217195CCD3BCB2ABEDF568713E14A"s,
     "7540EE1EDC44BB278D6EED5910943BE2CA3DE937B29400B5005FC03C47CE40DB"s,
     "76C7FB34C355FD6D79ADFAACFF797443BD970E52266EE6D8FCB0CB880ED1E713"s,
     "2931628CFDBAB1BB9349A8932CB6F10DB0E237C9CD17C284B0CDD1689A79FB3B"s,
     "BD5D3D03AE4188C166EB181C4D2F3DE40E78EEEF100D3CABE4BC8F0CDB18C207"s,
     "7029F7B04FCD0CABFC081120321C4C55B28C0842BD40E94FBCCEA51DA0CFF75A"s,
     "617C3E60837A9B0AA19ECBC7E30857BC211286C3BF48C81102B2560CDBEB95B9"s,
     "654C2DB91C8239C857D324EF988FABE37095F9455F3E5C37BCD6D2271ED6FF46"s,
     "B02F7A058B81B83359C8238606864DD53D2A5D350E77D32151F884D605395984"s,
     "D0FC8485C67B40E8A386D4EE940D1C38D8836EBFE89AF56456FDF220F720A20D"s,
     "B8D9C8E75BB82F6388A6EAD8AA3DF8BE8ED10FD73E637B6F98A1F77F517C3D57"s,
     "0E653795BF69072A26F16FC93AAEC0CE9FC1A4CD7188072200E00AED1A999B66"s,
     "953A922402E62A92FD2A9A26A200DF630F75B0A38460668E48EA7E30C21E17E7"s,
     "56BC71CA342AC7059BF4B10C514F87B2F92D0A9FC451154225C336EB01EDFDFF"s,
     "2B6067A1FFCE95C21DDB2F507710652937642B96FD7E01D7B4508960381B0BEC"s,
     "4E7F9F24F09EB5E69EC11DBD0FF0FEFE2D6965AAADB99122E81E81004BCEA5A0"s,
     "AFB7FE97396746C73B13AF7C338AD6E2CB32C12146460575BF90CAA28AB3F890"s,
     "7F6E46EC8ADAAB322712E64414070AB12896D43BE271C5F91D7822C8F2332460"s,
     "C9C9DCD6D7CAB11726F8E6D945A4A2690D3D0E3322951E6F213FD98FAEF69BC6"s,
     "5BA16435FB5EB5370E3BF495862D978DB85891AC34D0CD82664264EE304F8B4D"s,
     "7A22041DD76AB5AA2AC5BA45AD387FAA48F69E16315101FFDA6E76941FC6D1F0"s,
     "7B92479C8FC37424F1D89EB616E3F02A35DD3660586D38EE21B2AE5B061FA335"s,
     "3690EF8065D0FD204AAA9E6DB5425FEB5DA337CC099E45"s,
     }}},
    {            CryptoMethod::AES_256_GCM,
     {.salt_ = "000102030405060708090A0B0C0D0E0F0F0E0D0C0B0A09080706050403020100"s,
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
     "A6F26BA87A0756C1224833947023B15494E6B1682AA3DDE920641F4F7E0FC999"s,
     "EC195691D137D57307750B739AE4FBEE887AF2EE1A561EF6B363FD04ED52ED5A"s,
     "79E516FE1504F245FAC8F43EBD9D177E757D7F70623E155AA98911B868F9E2E2"s,
     "F906CF71C62A318EFDFA53E204AC0BD49744D6C8F6AA4B2B8B7B203C21D55EE4"s,
     "D38D71E5A02C844A0F2654C8D6A7166AA930DA0B04624FBA9CADC2AD201A32C7"s,
     "5DAFB0FB58390629141275DC5BFECE02C036490DB5E623751DACB4BC37ACA441"s,
     "F870D332EE95B9D9C6E8457D40D86DCCB29E1ADF817DD1952B340A78F20B5878"s,
     "A3971838F77713EF1876E35F2F04A0BBE5CCCB0C15609872D4665C2A68F4D6D5"s,
     "CDE4BBBDF208A41383AFB1CDDAF19882424AC57D34CB7A92A37945BB33E31465"s,
     "579819C86168110CD5008B5D675F1572081716690F6CD7DFBC9E42F4B8487356"s,
     "700DC2B45641EBAAA664DF9083A397967403C041770085C06EF7C1D03F095E44"s,
     "619179B3999574FB3FCA55D581E35580B0D3E878AD02EF0F03BDE799A695B124"s,
     "86A02B617B4B2D1449BCB6E4F82AEFEAC159FB94F2A6E77F901E655C189C107E"s,
     "FFCBE847DF46A2B241143BAE00ECD369AF32CB46B6102087D3440E2960BD9268"s,
     "029D29E3116F81ACF669BBA546E7F12BB13AB98BC85B3D5972412C4CD37E4446"s,
     "5748B7859371B99D9216CBFBFA185F3030C9B923E64BED86C07BF524DB51483D"s,
     "4EF4E0350B8022B791B850125A88CAA6CCE436148ABF2A83F6007D394FEDDD5C"s,
     "EA3E61B3C83A0AA9615583E46F86551B8AC540205576A0E7DB257546BABF4A22"s,
     "49846EB0F0992B2B4A701323A8BCB31887A0F0F538479406EBBFA14D9099321B"s,
     "57B11F052A1ED65B158CFC1299144F795F3C6EA6D05AC81CFA07663812BAF1ED"s,
     "8E59ED9AA14DEFC8C288329B03B431CAD8E7A8B85E9B0475AB7E49AF0405A305"s,
     "210A519BC4BE3B1AF280C5F2D5BFF6F844638EEE2CD9143D20A0CD35AADE713C"s,
     "5253ABC34C8BC97CA36096CB6CF80C77D18F36C66B78E2FDD5799C6D22A98D8D"s,
     "2284052CEAC971CEC204019F9ADFDA2AD44AB0C323C5AC"s,
     }}},
    {CryptoMethod::XCHACHA20_IETF_POLY1305,
     {.salt_    = "000102030405060708090A0B0C0D0E0F0F0E0D0C0B0A09080706050403020100"s,
     .ciphers_ = {
     "3F44BEAC0D3D42B71BEF662F52173755A78A53656F703D4049676CAE189D78F3"s,
     "C9CF8AF3E08185048BC17FCABC9BB05F0A66634A295E649DAE2223C4DD5A9CC3"s,
     "9FF38AC231666ACF79070C013CD68ADD359F087BB13C9EF2177635F582E800BD"s,
     "EFB51A219E8FAAD4D018774651F272299456DA6A2EAC4725B473F79EB8B02756"s,
     "F79FC3C18DA915D0A63085786802CDE0C0B6300A3BEC777A9E2D13C033E461D0"s,
     "296D57253DFC0D13B73BFD810AC4132930D52BAD10E6D24E1AE25BA6F00C46A0"s,
     "0C8FDE0134CD0BEC56C9F7D0FBAE1F6BFEB71A51249D99BDAA323641BF3427C8"s,
     "F4238BBC35A0361EA7407F23B223537DF068F32EFA1A7E479E8132D5675D3626"s,
     "6E8307EEEB5AE1954BDC388CD3A56CEFE02DF74323C74CF2188F38E83A6EE75E"s,
     "8943B8FA43400DEE67D3C1D302F54BEEBDEFA239984FC9288F193055E075FB51"s,
     "D40DAF8534644213819B907760974455F829DECEEE4B6B291282173D0748DD28"s,
     "5FF175B09A4DE25C2E06CEFB031FDE8FE4EFEC58630B18B3A70F7A5A38A4F822"s,
     "75A5DA9BA7B872DB3371997CADBCF86857117A1A29839CBEBE0D8494407CDDF1"s,
     "A7C66BDED695D2E553A0A0E2A8665F0B37355C10ADAB36B5C97EAA6C93E6845D"s,
     "FCCA914B988796920D90E136EFE28F143D5D1DB3148EA72EDD1EF451BD4B2CEE"s,
     "B6591972A53DEBFC09953A3EA29153AAA4471FE04147ECB1EE09B51BA6DE62FF"s,
     "D55DF17FA851C1DB2B3FC35F5062B61D4061FF395B4FBBFFC52F12290D0B644A"s,
     "00C5FAC5741216BB04C5C9830C02A90306CCD7B8661B7C894D5CAFB8FB4988E0"s,
     "A7A820BB2320880ECA3572CA3C5D4D94A5A6027F4C32ED50C30906637A06B424"s,
     "15551E29813F2CA3AD22BC041533F150F6949B7567F91E4E1B4DDE8B358D2B2D"s,
     "52F52A533462D814C91140C787426CB61E2A3BA4A204564F843F8EBC2EE3D2E9"s,
     "811243D8B64C90410B58DE6A5861F9825C45A1495D4FEA57897F040DCB956B0B"s,
     "6621C0430C43FB1D5DCA9E09EDB62480FC45C9DA699383E0B084FD882A4A8D16"s,
     "EFB71E1A892807CFB117B6664EFFE71B7FF39A8FF41A43"s,
     }}},
};

template <typename TestCase> void run_test_case(TestCase&& test)
{
  run_case([test = std::forward<TestCase>(test)](auto&& ex) -> Awaitable<void> {
    for (auto&& item : KEY_SIZE) {
      auto [m, s] = item;
      auto ec     = co_await redirect(test(ex, m, s));
      co_await service::get_sentry(ex).reset();
      BOOST_CHECK(!ec);
    }
  });
}

BOOST_AUTO_TEST_SUITE(SS)

BOOST_AUTO_TEST_CASE(SaltSentry_Conflict)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto ec = co_await redirect(send_salt(ex, m, s, 0_u8));
    BOOST_CHECK(!ec);

    ec = co_await redirect(send_salt(ex, m, s, 0_u8));
    BOOST_CHECK_EQUAL(make_error_code(PichiError::BAD_PROTO), ec);

    co_await service::get_sentry(ex).reset();
  });
}

BOOST_AUTO_TEST_CASE(SaltSentry_Normal)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    for (auto c : views::iota(0_u8, 0xff_u8)) {
      auto ec = co_await redirect(send_salt(ex, m, s, c));
      BOOST_CHECK(!ec);
    }

    co_await service::get_sentry(ex).reset();
  });
}

BOOST_AUTO_TEST_CASE(Encryption)
{
  rngs::for_each(TEST_DATA | views::keys, [](auto&& method) {
    auto encryptor = Encryptor{method, PASSWORD};
    auto decryptor = Decryptor{method};
    auto salt      = decryptor.salt();

    rngs::copy(encryptor.salt(), rngs::begin(salt));
    decryptor.set_psk(PASSWORD);

    auto fact = PLAINS | views::transform([&](auto&& plain) {
                  auto c = Buffer{};
                  auto n = encryptor.process(plain, c);
                  auto p = std::string(rngs::size(plain), 0);

                  decryptor.process({c, n}, p);
                  return p;
                });
    BOOST_CHECK(rngs::equal(PLAINS, fact));
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

    auto fact = td.ciphers_ | views::transform([&](auto&& cipher) {
                  auto c = Buffer{};
                  auto d = Buffer{};
                  auto p = ConstBuffer{d, decryptor.process({c, Botan::hex_decode(c, cipher)}, d)};
                  return std::string{rngs::begin(p), rngs::end(p)};
                });

    BOOST_CHECK(rngs::equal(PLAINS, fact));
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

BOOST_AUTO_TEST_CASE(Stream_accept)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto client = TestSocket{ex};
    auto stream = Stream{m, PASSWORD, client.peer()};

    co_await stream::write(client, SALT | views::take(s));

    auto dummy = "dummy"s;
    co_await stream::write(client, dummy);
    co_await stream::accept(stream);

    auto buf  = Buffer{};
    auto fact = buf | views::take(co_await stream::read_some(stream.next_layer(), buf));
    BOOST_CHECK(rngs::equal(dummy, fact));
  });
}

BOOST_AUTO_TEST_CASE(Stream_connect)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto salt = SALT | views::take(s);

    auto server = TestSocket{ex};
    auto stream = Stream{m, PASSWORD, salt, server.peer()};

    co_await stream::connect(stream, ENDPOINT);

    auto buf = Buffer{};

    auto fact   = Botan::hex_encode(buf | views::take(co_await stream::read_some(server, buf)));
    auto expect = std::format("{}{}", Botan::hex_encode(salt), DATA.at(m));

    BOOST_CHECK_EQUAL(expect, fact);
  });
}

BOOST_AUTO_TEST_CASE(Stream_read)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto salt = SALT | views::take(s);

    auto client = TestSocket{ex};
    auto stream = Stream{m, PASSWORD, client.peer()};

    auto buf = Buffer{};

    co_await stream::write(client, salt);
    co_await stream::write(client, buf | views::take(Botan::hex_decode(buf, DATA.at(m))));

    auto fact   = buf | views::take(co_await stream::read_some(stream, buf));
    auto expect = Buffer{};
    BOOST_CHECK(rngs::equal(expect | views::take(serializeEndpoint(ENDPOINT, expect)), fact));
  });
}

BOOST_AUTO_TEST_CASE(Stream_write)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto salt = SALT | views::take(s);

    auto server = TestSocket{ex};
    auto stream = Stream{m, PASSWORD, salt, server.peer()};

    auto buf = Buffer{};

    co_await stream::write(stream, buf | views::take(serializeEndpoint(ENDPOINT, buf)));

    auto fact   = Botan::hex_encode(buf | views::take(co_await stream::read_some(server, buf)));
    auto expect = std::format("{}{}", Botan::hex_encode(salt), DATA.at(m));

    BOOST_CHECK_EQUAL(expect, fact);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_read_remote)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{
        {m, PASSWORD, client.peer()}
    };

    auto buf = Buffer{};
    co_await stream::write(client, SALT | views::take(s));
    co_await stream::write(client, buf | views::take(Botan::hex_decode(buf, DATA.at(m))));

    auto fact = co_await ingress.read_remote();
    BOOST_CHECK(ENDPOINT.type_ == fact.type_);
    BOOST_CHECK_EQUAL(ENDPOINT.host_, fact.host_);
    BOOST_CHECK_EQUAL(ENDPOINT.port_, fact.port_);
  });
}

BOOST_AUTO_TEST_CASE(Ingress_confirm)
{
  run_test_case([](auto&& ex, auto m, auto) -> Awaitable<void> {
    auto client = TestSocket{ex};

    // Write a dummy string to prevent next asnyc_read from blocking
    auto dummy = "dummy"s;
    auto peer  = client.peer();
    co_await stream::write(peer, dummy);

    auto ingress = Ingress{
        {m, PASSWORD, std::move(peer)}
    };

    co_await ingress.confirm();

    auto buf = Buffer{};
    BOOST_CHECK(rngs::equal(dummy, buf | views::take(co_await stream::read_some(client, buf))));
  });
}

BOOST_AUTO_TEST_CASE(Ingress_disconnect)
{
  run_test_case([](auto&& ex, auto m, auto) -> Awaitable<void> {
    auto client = TestSocket{ex};

    // Write a dummy string to prevent next asnyc_read from blocking
    auto dummy = "dummy"s;
    auto peer  = client.peer();
    co_await stream::write(peer, dummy);

    auto ingress = Ingress{
        {m, PASSWORD, std::move(peer)}
    };

    co_await ingress.disconnect(makeErrorCode(PichiError::MISC));

    auto buf = Buffer{};
    BOOST_CHECK(rngs::equal(dummy, buf | views::take(co_await stream::read_some(client, buf))));
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{
        {m, PASSWORD, client.peer()}
    };

    auto salt = SALT | views::take(s);
    auto buf  = Buffer{};

    co_await stream::write(client, salt);
    co_await stream::write(client, buf | views::take(Botan::hex_decode(buf, DATA.at(m))));

    auto fact   = buf | views::take(co_await ingress.recv(buf));
    auto expect = Buffer{};
    BOOST_CHECK(rngs::equal(expect | views::take(serializeEndpoint(ENDPOINT, expect)), fact));
  });
}

BOOST_AUTO_TEST_CASE(Ingress_recv_With_Insufficient_Buffer)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto client  = TestSocket{ex};
    auto ingress = Ingress{
        {m, PASSWORD, client.peer()}
    };

    auto salt = SALT | views::take(s);
    auto buf  = Buffer{};

    co_await stream::write(client, salt);
    co_await stream::write(client, buf | views::take(Botan::hex_decode(buf, DATA.at(m))));

    auto expect = Buffer{};
    auto len    = serializeEndpoint(ENDPOINT, expect);

    for (auto i = 0_sz; i < len; ++i) {
      BOOST_CHECK_EQUAL(1, co_await ingress.recv(buf | views::drop(i) | views::take(1)));
    }
    BOOST_CHECK(rngs::equal(expect | views::take(len), buf | views::take(len)));
  });
}

BOOST_AUTO_TEST_CASE(Ingress_send)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto salt = SALT | views::take(s);

    auto client  = TestSocket{ex};
    auto ingress = Ingress{
        {m, PASSWORD, salt, client.peer()}
    };

    auto buf = Buffer{};

    co_await ingress.send(buf | views::take(serializeEndpoint(ENDPOINT, buf)));

    auto fact   = Botan::hex_encode(buf | views::take(co_await stream::read_some(client, buf)));
    auto expect = std::format("{}{}", Botan::hex_encode(salt), DATA.at(m));
    BOOST_CHECK_EQUAL(expect, fact);
  });
}

BOOST_AUTO_TEST_CASE(Egress_connect)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto salt = SALT | views::take(s);

    auto server = TestSocket{ex};
    auto egress = Egress{
        {m, PASSWORD, salt, server.peer()}
    };

    co_await egress.connect(ENDPOINT);

    auto buf    = Buffer{};
    auto fact   = Botan::hex_encode(buf | views::take(co_await stream::read_some(server, buf)));
    auto expect = std::format("{}{}", Botan::hex_encode(salt), DATA.at(m));
    BOOST_CHECK_EQUAL(expect, fact);
  });
}

BOOST_AUTO_TEST_CASE(Egress_recv)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto salt = SALT | views::take(s);

    auto server = TestSocket{ex};
    auto egress = Egress{
        {m, PASSWORD, salt, server.peer()}
    };

    auto buf = Buffer{};

    co_await stream::write(server, salt);
    co_await stream::write(server, buf | views::take(Botan::hex_decode(buf, DATA.at(m))));

    auto fact   = buf | views::take(co_await egress.recv(buf));
    auto expect = Buffer{};
    BOOST_CHECK(rngs::equal(expect | views::take(serializeEndpoint(ENDPOINT, expect)), fact));
  });
}

BOOST_AUTO_TEST_CASE(Egress_recv_With_Insufficient_Buffer)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto salt = SALT | views::take(s);

    auto server = TestSocket{ex};
    auto egress = Egress{
        {m, PASSWORD, salt, server.peer()}
    };

    auto buf = Buffer{};

    co_await stream::write(server, salt);
    co_await stream::write(server, buf | views::take(Botan::hex_decode(buf, DATA.at(m))));

    auto expect = Buffer{};
    auto len    = serializeEndpoint(ENDPOINT, expect);

    for (auto i = 0_sz; i < len; ++i) {
      BOOST_CHECK_EQUAL(1, co_await egress.recv(buf | views::drop(i) | views::take(1)));
    }
    BOOST_CHECK(rngs::equal(expect | views::take(len), buf | views::take(len)));
  });
}

BOOST_AUTO_TEST_CASE(Egress_send)
{
  run_test_case([](auto&& ex, auto m, auto s) -> Awaitable<void> {
    auto salt = SALT | views::take(s);

    auto server = TestSocket{ex};
    auto egress = Egress{
        {m, PASSWORD, salt, server.peer()}
    };

    auto buf = Buffer{};

    co_await egress.send(buf | views::take(serializeEndpoint(ENDPOINT, buf)));

    auto fact   = Botan::hex_encode(buf | views::take(co_await stream::read_some(server, buf)));
    auto expect = std::format("{}{}", Botan::hex_encode(salt), DATA.at(m));
    BOOST_CHECK_EQUAL(expect, fact);
  });
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
