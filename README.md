# Pichi ![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg)

Pichi is an application layer proxy, which can be fully controlled via RESTful APIs.

## Build Status

| OS | macOS 10.13 | Alpine 3.8 | Windows 10 |
|:-------------:|:-----------------------------------------------------------------------------------------------------------------------:|:-----------------------------------------------------------------------------------------------------------------------:|:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|
| **Toolchain** | Clang 6.0.x | GCC 8.x | VC++2017 |
| **Status** | [![Build Status](https://travis-ci.org/pichi-router/pichi.svg?branch=master)](https://travis-ci.org/pichi-router/pichi) | [![Build Status](https://travis-ci.org/pichi-router/pichi.svg?branch=master)](https://travis-ci.org/pichi-router/pichi) | [![Build Status](https://ci.appveyor.com/api/projects/status/github/pichi-router/pichi?branch=appveyor&svg=true)](https://ci.appveyor.com/api/projects/status/github/pichi-router/pichi?branch=appveyor&svg=true) |

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
* [Boost](https://www.boost.org)
* [MbedTLS](https://tls.mbed.org)
* [libsodium](https://libsodium.org)
* [RapidJSON](http://rapidjson.org/)
* [libmaxminddb](http://maxmind.github.io/libmaxminddb/)

### Build and run tests

Build and run on *nix:

```
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ cmake --build .
$ ctest --output-on-failure
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
  -h [ --help ]              produce help message
  -l [ --listen ] arg (=::1) API server address
  -p [ --port ] arg          API server port
  -g [ --geo ] arg           GEO file
$ docker run -d --name pichi --net host --restart always pichi/pichi \
>   pichi -g /usr/share/pichi/geo.mmdb -p 1024 -l 127.0.0.1
c51b832bd29dd0333b0d32b0b0563ddc72821f7301c36c7635ae47d00a3bb902
$ docker ps -n 1
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS               NAMES
c51b832bd29d        pichi/pichi         "pichi -g /usr/share…"   1 seconds ago       Up 1 seconds                            pichi
```
