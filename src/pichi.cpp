#include "pichi/common/config.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <pichi.h>
#include <pichi/actor/detached.hpp>
#include <pichi/actor/server.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/mmdb.hpp>

namespace actor = pichi::actor;
namespace asio  = boost::asio;

static asio::io_context io{};

int pichi_run_server(char const* bind, uint16_t port, char const* mmdb)
{
  try {
    pichi::assertFalse(bind == nullptr);

    if (mmdb != nullptr) asio::use_service<pichi::Mmdb>(io).initialize(mmdb);

    auto server = actor::Server{io.get_executor()};
    asio::co_spawn(io, server.serve({asio::ip::make_address(bind), port}), actor::detached);

    io.run();
    return 0;
  }
  catch (std::exception const& e) {
    std::clog << std::format("ERROR: {}\n", e.what());
    return -1;
  }
}
