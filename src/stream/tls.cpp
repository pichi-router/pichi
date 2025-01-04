#include <pichi/common/config.hpp>
// Include config.hpp first
#include <pichi/stream/tls.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION >= 107300
#include <boost/asio/ssl/host_name_verification.hpp>
#else  // BOOST_VERSION >= 107300
#include <boost/asio/ssl/rfc2818_verification.hpp>
#endif  // BOOST_VERSION >= 107300

namespace asio = boost::asio;
namespace ssl  = asio::ssl;

namespace pichi::stream {

ssl::context tls_context(vo::TlsIngressOption const& opt)
{
  auto ctx = ssl::context{ssl::context::tls_server};
  ctx.use_certificate_chain_file(opt.certFile_);
  ctx.use_private_key_file(opt.keyFile_, ssl::context::pem);
  crypto::enableBrotliCompression(ctx.native_handle());
  return ctx;
}

ssl::context tls_context(vo::TlsEgressOption const& opt, std::string const& sn)
{
  auto ctx = ssl::context{ssl::context::tls_client};
  crypto::setupTlsFingerprint(ctx.native_handle());
  if (opt.insecure_) {
    ctx.set_verify_mode(ssl::context::verify_none);
    return ctx;
  }

  ctx.set_verify_mode(ssl::context::verify_peer);
  if (opt.caFile_.has_value())
    ctx.load_verify_file(*opt.caFile_);
  else {
    ctx.set_default_verify_paths();
#if BOOST_VERSION >= 107300
    ctx.set_verify_callback(ssl::host_name_verification{opt.serverName_.value_or(sn)});
#else   // BOOST_VERSION >= 107300
    ctx.set_verify_callback(ssl::rfc2818_verification{option.serverName_.value_or(serverName)});
#endif  // BOOST_VERSION >= 107300
  }
  return ctx;
}

}  // namespace pichi::stream
