---
title: Integration
nav_order: 5
permalink: /integration
---

# Integration

There are 2 ways to integrate with Pichi:

* **Standalone**: Pichi runs in its own process,
* **In-Process**: Pichi runs in its supervisor process.

Regardless of any mode, the supervisor must communicate with pichi via RESTful APIs.

## Standalone

Standalone mode requires `BUILD_SERVER` CMake option, which builds code in `server` directory. For example:

```bash
$ cmake -D CMAKE_INSTALL_PREFIX=/usr -D CMAKE_BUILD_TYPE=MinSizeRel -D BUILD_SERVER=ON -B build .
$ cmake --build build --target install/strip
```

## In-Process

In-Process mode is suitable for the scenarios that the standalone process is prohibited or unnecessary, or the supervisor prefers to run Pichi in its own process.
There are 2 types of interface to run pichi.

### C function

C function can be invoked by lots of program languages. It's defined in [include/pichi.h](https://github.com/pichi-router/pichi/blob/main/include.pichi.h):

```c
/*
 * Start PICHI server according to
 *   - bind: server listening address, NOT NULL,
 *   - port: server listening port,
 *   - mmdb: IP GEO database, MMDB format.
 * The function doesn't return if no error occurs, otherwise -1.
 */
extern int pichi_run_server(char const* bind, uint16_t port, char const* mmdb);
```

{: .important }
> `pichi_run_server` will **block** the caller thread if no error occurs.

### C++ class

C++ class might sometimes be friendly while the supervisor is written in C++. It's defined in [include/pichi/actor/server.hpp](https://github.com/pichi-router/pichi/blob/main/include/pichi/actor/server.hpp):

```c++
namespace pichi::actor {

class Server {
public:
  Server(boost::asio::any_io_executor const&);
  boost::asio::awaitable<void> serve(boost::asio::ip::tcp::endpoint);
};

} // namespace pichi::actor
```

`Server::serve` accepts a local endpoint and then starts listening on it to accept [RESTful APIs](./api-specification) requests.
It returns a [boost::asio::awaitable](https://www.boost.org/doc/libs/latest/doc/html/boost_asio/reference/awaitable.html) class template,
which is usually passed to [boost::asio::co_spawn](https://www.boost.org/doc/libs/latest/doc/html/boost_asio/reference/co_spawn.html) function template directly.

{: .note }
> [pichi::service::Mmdb](https://github.com/pichi-router/pichi/blob/develop/include/pichi/service/mmdb.hpp) is an optional service.
> If MMDB is NOT initialized, Pichi will DISABLE all `country` and `country_nr` matchings.

```c++
#include <pichi/actor/server.hpp>
#include <pichi/service/mmdb.hpp>

auto io = boost::asio::io_context{};
auto ep = boost::asio::ip::endpoint{boost::asio::ip::make_address(addr), port};

// Optional: Initialize MMDB service
boost::asio::use_service<pichi::service::Mmdb>(io).initialize(mmdb_fname);

auto svr = pichi::actor::Server{io.get_executor()};
boost::asio::co_spawn(io, svr.serve(ep), boost::asio::detached);

// Setup other ASIO services

io.run();  // Thread blocked

```
