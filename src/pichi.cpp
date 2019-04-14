#include <boost/asio/io_context.hpp>
#include <iostream>
#include <pichi.h>
#include <pichi/api/server.hpp>
#include <pichi/asserts.hpp>

using namespace std;
namespace api = pichi::api;
namespace asio = boost::asio;

static asio::io_context io{1};

int pichi_run_server(char const* bind, uint16_t port, char const* mmdb)
{
  try {
    pichi::assertFalse(bind == nullptr);
    pichi::assertFalse(mmdb == nullptr);
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
