#include <boost/asio/io_context.hpp>
#include <iostream>
#include <pichi.h>
#include <pichi/api/server.hpp>

using namespace std;
namespace api = pichi::api;
namespace asio = boost::asio;

static asio::io_context io{1};

char const* PICHI_DEFAULT_BIND = "::1";
char const* PICHI_DEFAULT_MMDB = "/usr/share/pichi/geo.mmdb";

int pichi_run_server(char const* bind, uint16_t port, char const* mmdb)
{
  try {
    if (bind == nullptr) bind = PICHI_DEFAULT_BIND;
    if (mmdb == nullptr) mmdb = PICHI_DEFAULT_MMDB;
    auto server = api::Server{io, mmdb};
    server.listen(bind, port);
    io.run();
    return 0;
  }
  catch (exception const& e) {
    cout << "ERROR: " << e.what() << endl;
    return -1;
  }
}
