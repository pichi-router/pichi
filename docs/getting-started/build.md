---
title: Build
nav_order: 1
parent: Getting Started
permalink: /getting-started/build
---

# Build with CMake

Pichi requires [CMake](https://cmake.org/) 3.15.0 or later to manage the build process, and a C++ toolchain that supports the **C++20** standard.

## Dependencies

| Library | Comment |
|:---|:---|
| [Boost](https://www.boost.org) | 1.77.0 or later |
| [Botan](https://botan.randombit.net/) | 3.5.0 or later |
| [RapidJSON](http://rapidjson.org/) | 1.1.0 or later |
| [Libmaxminddb](http://maxmind.github.io/libmaxminddb/) | 1.3.0 or later |
| [OpenSSL](https://www.openssl.org) | if `TLS_FINGERPRINT` disabled |
| [BoringSSL](https://boringssl.googlesource.com/boringssl/) | if `TLS_FINGERPRINT` enabled |

## CMake options

* `BUILD_SERVER`: Build pichi application, the default is **ON**.
* `BUILD_TEST`: Build unit test cases, the default is **ON**.
* `STATIC_LINK`: Generate static library, the default is **ON**.
* `INSTALL_DEVEL`: Install development files, the default is **OFF**.
* `TRANSPARENT_PF`: Build the transparent ingress implemented by PF, the default is **OFF**.
* `TRANSPARENT_IPTABLES`: Build the transparent ingress implemented by iptables, the default is **OFF**.
* `TLS_FINGERPRINT`: Enable TLS fingerprint simulation, which requiring [BoringSSL](https://boringssl.googlesource.com/boringssl/), the default is **ON**.

## Build and run tests with CMake

```bash
$ cmake [CMake options] -B /path/to/build -S /path/to/pichi
$ cmake --build /path/to/build
$ cmake --build /path/to/build --target test
```

# Build with Conan

Because Pichi depends on multiple C/C++ libraries, handling dependency management directly with CMake can be complicated,
particularly in cross-compilation scenario.
To simplify it, Pichi utilize [Conan](https://conan.io), which is a powerful C/C++ package manager.
Please refer to its [documentation](https://docs.conan.io/2/) before getting started.

## Export recipes

Pichi customized several recipes because of following reasons:

* BoringSSL: ConanCenter doesn't have it.
* Botan: Building failure on FreeBSD.
* Libmaxminddb: Cross-compiling failure.

Therefore, it's necessary to export them to local cache before building Pichi.

```bash
$ conan export --version 42 .conan/recipes/boringssl
$ conan export --version 3.12.0 .conan/recipes/botan
$ conan export --version 1.12.2 .conan/recipes/libmaxminddb
```

## Build

### Native building

```bash
$ conan create [Conan options] .
$ ~/.conan2/p/b/pichi0e609cdeeeb38/p/bin/pichi
Allow options:
  -h [ --help ]                   produce help message
  -l [ --listen ] arg (=::1)      API server address
  -p [ --port ] arg               API server port
  -g [ --geo ] arg                GEO file
  --json arg                      Initial configration(JSON format)
  -v [ --version ]                show version
  -d [ --daemon ]                 daemonize
  --pid arg (=/var/run/pichi.pid) pid file
  --log arg (=/var/log/pichi.log) log file
  -u [ --user ] arg               run as user
  --group arg                     run as group
```

### Cross compilation for Android

```bash
$ cat ~/.conan2/profiles/android
[settings]
os=Android
os.api_level=35
arch=armv8
build_type=Release
compiler=clang
compiler.version=21
compiler.libcxx=c++_static
[tool_requires]
andoird-ndk/r29
[conf]
tools.android:ndk_path=~/.conan2/p/andro9c0a9d0845952/p/bin
$ conan create [Conan options] -pr android .
```

### Cross compilation for iOS

```bash
$ cat ~/.conan2/profiles/iphoneos
[settings]
os=iOS
os.sdk=iphoneos
os.version=26.0
arch=armv8
build_type=Release
compiler=apple-clang
compiler.libcxx=libc++
compiler.version=17
$ conan create [Conan options] -pr iphoneos .
```

### As a developer

```bash
$ # Resolve and download dependencies, then generate CMake toolchain file
$ conan install [Conan options] -s 'pichi/*:build_type=Debug' .
$
$ # Configure
$ cmake --preset conan-default
$
$ # Build
$ cmake --build build/Debug
```

# Just

While Conan is well-suited for managing dependencies, remembering all of its specific commands can be chanllenging.
To simplify this, Pichi uses [Just](https://just.systems/), a handy way to save and run project-specific commands.
The provided [.justfile](https://github.com/pichi-router/pichi/blob/main/.justfile) automates these tasks,
streamlining both daily local development and the [GitHub Actions](https://github.com/features/actions) workflows.

```bash
$ just help
Available recipes:
    build version="latest" build_type="Release" shared="False"
    build-android version="latest" ndk_ver="r29" arch="armv8" build_type="Release"
    build-ios version="latest" sdk="iphoneos" build_type="Release"
    clean *recipes
    detect-android ndk_ver
    detect-host
    dev build_type="Debug" shared="False" version="latest"
    dev-android build_type="Debug" ndk_ver="r29" arch="armv8" version="latest"
    dev-ios build_type="Debug" sdk="iphoneos" version="latest"
    export
    help
    remove *recipes
$
$ # Build from scratch
$ just build
$
$ # Establish the development environment from scratch
$ just dev
$ cmake --preset conan-debug
$ cmake --build build/Debug
```
