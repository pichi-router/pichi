#include <boost/program_options.hpp>
#include <iostream>
#include <pichi.h>

using namespace std;
namespace po = boost::program_options;

int main(int argc, char const* argv[])
{
  auto listen = string{};
  auto port = uint16_t{};
  auto geo = string{};
  auto desc = po::options_description{"Allow options"};
  desc.add_options()("help,h", "produce help message")(
      "listen,l", po::value<string>(&listen)->default_value(PICHI_DEFAULT_BIND),
      "API server address")("port,p", po::value<uint16_t>(&port), "API server port")(
      "geo,g", po::value<string>(&geo)->default_value(PICHI_DEFAULT_MMDB), "GEO file");
  auto vm = po::variables_map{};
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("port") || !vm.count("geo")) {
    cout << desc << endl;
    return 1;
  }

  return pichi_run_server(listen.c_str(), port, geo.c_str());
}
