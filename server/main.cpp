#include <boost/asio/io_context.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <pichi/api/server.hpp>

using namespace std;
namespace api = pichi::api;
namespace asio = boost::asio;
namespace po = boost::program_options;

static auto io = asio::io_context{1};

int main(int argc, char const* argv[])
{
  auto listen = string{};
  auto port = uint16_t{};
  auto geo = string{};
  auto desc = po::options_description{"Allow options"};
  desc.add_options()("help,h", "produce help message")(
      "listen,l", po::value<string>(&listen)->default_value("::1"),
      "API server address")("port,p", po::value<uint16_t>(&port),
                            "API server port")("geo,g", po::value<string>(&geo), "GEO file");
  auto vm = po::variables_map{};
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("port") || !vm.count("geo")) {
    cout << desc << "\n";
    return 1;
  }

  auto server = api::Server{io, geo.c_str()};
  server.listen(listen, port);

  io.run();
  return 0;
}
