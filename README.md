# Pichi

Pichi is an application layer proxy, which can be fully controlled via RESTful APIs.

| Clang6.0.1/macOS 	| GCC8.1.0/Ubuntu14.04 	| VC++2017/Windows  	|
|:-----------------------------------------------------------------------------------------------------------------------:	|:-----------------------------------------------------------------------------------------------------------------------:	|:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:	|
| [![Build Status](https://travis-ci.org/pichi-router/pichi.svg?branch=master)](https://travis-ci.org/pichi-router/pichi) 	| [![Build Status](https://travis-ci.org/pichi-router/pichi.svg?branch=master)](https://travis-ci.org/pichi-router/pichi) 	| [![Build Status](https://ci.appveyor.com/api/projects/status/github/pichi-router/pichi?branch=master&svg=true)](https://ci.appveyor.com/api/projects/status/github/pichi-router/pichi?branch=master&svg=true) 	|

# Using Pichi API

## Resources

* **Inbound**: defines an incoming network adapter, containing protocol, listening address/port and protocol specific configurations.
* **Outbound**: defines an outgoing network adapter, containing protocol, next hop address/port and protocol specific configurations.
* **Rule**: contains one outbound adapter name and a group of conditions, such as IP range, domain regular expression, the country of the destination IP, and so on, that the incoming connection matching ANY conditions would be forwarded to the specified outbound.
* **Route**: indicates a priority ordered rules, and a default outbound which would be forwarded to if none of the rules matched.

## API Specification

[Pichi API](https://app.swaggerhub.com/apis/pichi-router/pichi-api/1.0)

## Examples

### Proxy like ss-local(shadowsocks-libev)

```
$ curl -i -X PUT -d '{"type":"socks5","bind":"127.0.0.1","port":1080}' http://pichi-router:port/inbound/socks5
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"type":"ss","host":"my-ss-server","port":8388,"method":"rc4-md5","password":"my-password"}' http://pichi-router:port/outbound/shadowsocks
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"default":"shadowsocks"}' http://pichi-router:port/route
HTTP/1.1 204 No Content

```

### HTTP proxy except intranet

```
$ curl -i -X PUT -d '{"type":"http","bind":"::","port":8080}' http://pichi-router:port/inbound/http
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"type":"http","host":"http-proxy","port":8080}' http://pichi-router:port/outbound/http
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"outbound":"direct","range":["::1/128","127.0.0.1/32", "10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16", "fc00::/7"],"domain":["local"],"pattern":["^localhost$"]}' http://pichi-router:port/rules/intranet
HTTP/1.1 204 No Content

$ curl -i -X PUT -d '{"default":"http","rules":["intranet"]}' http://pichi-router:port/route
HTTP/1.1 204 No Content

```

### 100 shadowsocks servers

```
$ for((i=20000;i<20100;++i)); do \
>   curl -X PUT \
>   -d "{\"type\":\"ss\",\"bind\":\"::\",\"port\":$i,\"method\":\"rc4-md5\",\"password\":\"pw-$i\"}" \
>   "http://pichi-router:port/inbound/$i"; \
> done

```

# Supported protocols

## Inbound protocols

* HTTP Proxy: defined by [RFC 2068](https://www.ietf.org/rfc/rfc2068.txt)
* HTTP Tunnel: defined by [RFC 2616](https://www.ietf.org/rfc/rfc2817.txt)
* SOCKS5: defined by [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt)
* Shadowsocks: defined by [shadowsocks.org](https://shadowsocks.org/en/spec/Protocol.html)

## Outbound protocols

* HTTP Tunnel: defined by [RFC 2616](https://www.ietf.org/rfc/rfc2817.txt)
* SOCKS5: defined by [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt)
* Shadowsocks: defined by [shadowsocks.org](https://shadowsocks.org/en/spec/Protocol.html)

# Build

## Requirements

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
$ ctest -C Debug --output-on-failure
```

Dockerfile based on [alpine](https://alpinelinux.org) can be found at the [Gist](https://gist.github.com/pichi-router/b8a6e3d04bf4d97339f1d40c017ce000).

Build and run on Windows with [Vcpkg](https://github.com/Microsoft/vcpkg):

```
PS C:\pichi> mkdir build
PS C:\pichi> cd build
PS C:\pichi\build> cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug ..
PS C:\pichi\build> cmake --build .
PS C:\pichi\build> ctest -C Debug --output-on-failure
```
