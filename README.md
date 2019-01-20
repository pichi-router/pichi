# Pichi ![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg)

Pichi is an application layer proxy, which can be fully controlled via RESTful APIs.

## Build Status

| OS | macOS 10.13 | Alpine 3.8 | Windows 10 | iOS 12.1 | Android 9 |
|:----------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------:|:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------------------------------------------:|
| **Toolchain** | Xcode 10.1 | GCC 8.x | VC++2017 | Xcode 10.1 | NDK r18b |
| **Architecture** | x86_64 | x86_64 | x86_64 | arm64/arm64e | arm64 |
| **Status** | [![Build Status](https://travis-matrix-badges.herokuapp.com/repos/pichi-router/pichi/branches/master/5)](https://travis-ci.org/pichi-router/pichi) | [![Build Status](https://travis-matrix-badges.herokuapp.com/repos/pichi-router/pichi/branches/master/3)](https://travis-ci.org/pichi-router/pichi) | [![Build Status](https://ci.appveyor.com/api/projects/status/github/pichi-router/pichi?branch=appveyor&svg=true)](https://ci.appveyor.com/api/projects/status/github/pichi-router/pichi?branch=appveyor&svg=true) | [![Build Status](https://travis-matrix-badges.herokuapp.com/repos/pichi-router/pichi/branches/master/1)](https://travis-ci.org/pichi-router/pichi) | [![Build Status](https://travis-matrix-badges.herokuapp.com/repos/pichi-router/pichi/branches/master/2)](https://travis-ci.org/pichi-router/pichi) |

## Using Pichi API

### Resources

* **Ingress**: defines an incoming network adapter, containing protocol, listening address/port and protocol specific configurations.
* **Egress**: defines an outgoing network adapter, containing protocol, next hop address/port and protocol specific configurations.
* **Rule**: contains one egress name and a group of conditions, such as IP range, domain regular expression, the country of the destination IP, and so on, that the incoming connection matching ANY conditions would be forwarded to the specified egress.
* **Route**: indicates a priority ordered rules, and a default egress which would be forwarded to if none of the rules matched.

### API Specification

[Pichi API](https://app.swaggerhub.com/apis/pichi-router/pichi-api/1.1)

### Examples

#### Proxy like ss-local(shadowsocks-libev)

```
$ curl -i -X PUT -d '{"type":"socks5","bind":"127.0.0.1","port":1080}' http://pichi-router:port/ingresses/socks5
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"type":"ss","host":"my-ss-server","port":8388,"method":"rc4-md5","password":"my-password"}' http://pichi-router:port/egresses/shadowsocks
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"default":"shadowsocks"}' http://pichi-router:port/route
HTTP/1.1 204 No Content

```

#### HTTP proxy except intranet

```
$ curl -i -X PUT -d '{"type":"http","bind":"::","port":8080}' http://pichi-router:port/ingresses/http
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"type":"http","host":"http-proxy","port":8080}' http://pichi-router:port/egresses/http
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"range":["::1/128","127.0.0.1/32", "10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16", "fc00::/7"],"domain":["local"],"pattern":["^localhost$"]}' http://pichi-router:port/rules/intranet
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"default":"http","rules":[["intranet","direct"]]}' http://pichi-router:port/route
HTTP/1.1 204 No Content

```

#### 100 shadowsocks servers

```
$ for((i=20000;i<20100;++i)); do \
>   curl -X PUT \
>   -d "{\"type\":\"ss\",\"bind\":\"::\",\"port\":$i,\"method\":\"rc4-md5\",\"password\":\"pw-$i\"}" \
>   "http://pichi-router:port/ingresses/$i"; \
> done

```

## Supported protocols

### Ingress protocols

* HTTP Proxy: defined by [RFC 2068](https://www.ietf.org/rfc/rfc2068.txt)
* HTTP Tunnel: defined by [RFC 2616](https://www.ietf.org/rfc/rfc2817.txt)
* SOCKS5: defined by [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt)
* Shadowsocks: defined by [shadowsocks.org](https://shadowsocks.org/en/spec/Protocol.html)

### Egress protocols

* HTTP Tunnel: defined by [RFC 2616](https://www.ietf.org/rfc/rfc2817.txt)
* SOCKS5: defined by [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt)
* Shadowsocks: defined by [shadowsocks.org](https://shadowsocks.org/en/spec/Protocol.html)

## Build

### Requirements

* C++17
* [Boost](https://www.boost.org) 1.67.0
* [MbedTLS](https://tls.mbed.org) 2.7.0
* [libsodium](https://libsodium.org) 1.0.12
* [RapidJSON](http://rapidjson.org/) 1.1.0
* [libmaxminddb](http://maxmind.github.io/libmaxminddb/) 1.3.0

### CMake options

* `BUILD_SERVER`: Build pichi application, the default is **ON**.
* `BUILD_TEST`: Build unit test cases, the default is **ON**.
* `STATIC_LINK`: Generate static library, the default is **ON**.
* `INSTALL_HEADERS`: Install header files, the default is **OFF**.

### Build and run tests

Build and run on Unix-like:

```
$ cmake -DCMAKE_BUILD_TYPE=MinSizeRel -B build .
$ cmake --build build
$ cmake --build build --target test
$ cmake --build build --target install/strip
```

Build and run on Windows with [Vcpkg](https://github.com/Microsoft/vcpkg):

```
PS C:\pichi> mkdir build
PS C:\pichi> cd build
PS C:\pichi\build> cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
PS C:\pichi\build> cmake --build . --config Debug
PS C:\pichi\build> ctest -C Debug --output-on-failure
```

### Docker

The pre-built docker image can be found on [Docker Store](https://store.docker.com/community/images/pichi/pichi),
which is automatically generated via `docker/pichi.dockerfile`. Furthermore, `docker/builder.dockerfile`
is intended to provide a docker environment for development.

```
$ docker pull pichi/pichi
$ docker run --rm pichi/pichi pichi -h
Allow options:
  -h [ --help ]                         produce help message
  -l [ --listen ] arg (=::1)            API server address
  -p [ --port ] arg                     API server port
  -g [ --geo ] arg (=/usr/share/pichi/geo.mmdb)
                                        GEO file
$ docker run -d --name pichi --net host --restart always pichi/pichi \
>   pichi -g /usr/share/pichi/geo.mmdb -p 1024 -l 127.0.0.1
c51b832bd29dd0333b0d32b0b0563ddc72821f7301c36c7635ae47d00a3bb902
$ docker ps -n 1
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS               NAMES
c51b832bd29d        pichi/pichi         "pichi -g /usr/share…"   1 seconds ago       Up 1 seconds                            pichi
```

### Build library for iOS/Android

Pichi is designed to run or be embeded into some APPs on iOS/Adnroid. `deps-build` directory gives some helping scripts to build Pichi's dependencies for iOS/Android.

#### iOS

It's very simple to build a C/C++ project managed by CMake, if `CMAKE_TOOLCHAIN_FILE` is set to [ios.toolchain.cmake](https://github.com/leetal/ios-cmake/blob/2.1.4/ios.toolchain.cmake).

```
$ cmake -D CMAKE_TOOLCHAIN_FILE=/path/to/ios.toolchain.cmake \
>   -D IOS_PLATFORM=OS -D IOS_ARCH=arm64 [other options] /path/to/project
```

On the other hand, `deps-build/boost.sh` can generate libraries for iOS if below environment variables are set:

* **PLATFORM**: to specify target OS(*iphoneos, iphonesimulator, appletvos, appletvsimulator*),
* **IOS_ROOT**: to specify root install directory of headers/libraries.

For example:

```
$ # In macOS with Xcode 10.0 or above
$ export PLATFORM=iphoneos
$ export IOS_ROOT=/path/to/ios/root
$ bash deps-build/boost.sh
Usage: boost.sh <src path>
$ bash deps-build/boost.sh /path/to/boost
...
```

#### Android

The usage of `deps-build/boost.sh` is very similar to iOS one, except environment varialbes:

* **PLATFORM**: *android-\<API\>s* are available,
* **ANDROID_ROOT**: to specify root install directory of headers/libraries.

Android NDK kindly provides `build/tools/make_standalone_toolchain.py` script to generate a cross-compiling toolchain for any version of Android.

```
$ export NDK_ROOT=/path/to/ndk
$ export TOOLCHAIN_ROOT=/path/to/toolchain
$ python ${NDK_ROOT}/build/tools/make_standalone_toolchain.py --arch arm64 --api 28 \
>   --stl libc++ --install-dir ${TOOLCHAIN_ROOT}
$ cmake -D CMAKE_SYSROOT=${TOOLCHAIN_ROOT}/sysroot \
>   -D CMAKE_C_COMPILER=${TOOLCHAIN_ROOT}/bin/clang \
>   -D CMAKE_CXX_COMPILER=${TOOLCHAIN_ROOT}/bin/clang++ \
>   [other options] /path/to/project
```

## Run pichi server

There are 2 ways to start pichi server:

* **Standalone**: pichi server runs in its own process,
* **In process**: pichi server runs in its supervisor process.

### Standalone

Standalone mode requires `BUILD_SERVER` CMake option, which builds code in `server` directory. For example:

```
$ cmake -D CMAKE_INSTALL_PREFIX=/usr -D CMAKE_BUILD_TYPE=MinSizeRel -D BUILD_SERVER=ON -B build .
$ cmake --build build --target install/strip
$ /usr/bin/pichi -h
Allow options:
  -h [ --help ]                         produce help message
  -l [ --listen ] arg (=::1)            API server address
  -p [ --port ] arg                     API server port
  -g [ --geo ] arg (=/usr/share/pichi/geo.mmdb)
                                        GEO file
```

### In process

InProcess mode is suitable for the scenarios that the standalone process is prohibited or unnecessary, such as iOS/Android, or the supervisor prefers to run pichi in its own process. There are 2 types of interface to run pichi.

#### C function

C function can be invoked by lots of program languages. It's defined in `include/pichi.h`:

```C
/*
 * Start PICHI server according to
 *   - bind: server listening address, PICHI_DEFAULT_BIND if NULL,
 *   - port: server listening port,
 *   - mmdb: IP GEO database, MMDB format, PICHI_DEFAULT_MMDB if NULL.
 * The function doesn't return if no error occurs, otherwise -1.
 */
extern int pichi_run_server(char const* bind, uint16_t port, char const* mmdb);
```

`pichi_run_server` will block the calling thread if no error occurs.

#### C++ class

C++ class might sometimes be friendly while the supervisor is written in C++. It's defined in `include/pichi/api/server.hpp`:

```C++
class Server {
public:
  Server(boost::asio::io_context&, char const* mmdb);
  void listen(std::string_view bind, uint16_t port);
};
```

`pichi::api::Server` accepts a `boost::asio::io_context` object reference, which is shared by the supervisor. Furthermore, `Server::listen` **doesn't** block the calling thread. It means that the supervisor can invoke `io_context::run()` right where it wants to do. Here's a simple code snippet:

```C++
#include <pichi/api/server.hpp>

auto io = boost::asio::io_context{};

auto server = pichi::api::Server{io, mmdb};
server.listen(bind, port);

// Setup other ASIO services

io.run();  // Thread blocked

```
