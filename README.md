# Pichi ![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg)

**Pichi** is a flexible rule-based proxy.

[Documentation](https://pichi-router.github.io/pichi/)

# Build Status

| OS | Ubuntu 24.04 | macOS 26 | FreeBSD 15.0 | Windows Server 2025 | Android | iOS |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Toolchain** | GCC 13.3.0 | Xcode 26.4.1 | Clang 19.1.7 | Visual Studio 2022 | Android NDK r29 | Xcode 26.4.1 |
| **Status** | [![Linux](https://github.com/pichi-router/pichi/actions/workflows/linux.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/linux.yml) | [![macOS](https://github.com/pichi-router/pichi/actions/workflows/macos.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/macos.yml) | [![FreeBSD](https://github.com/pichi-router/pichi/actions/workflows/freebsd.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/freebsd.yml) | [![Windows](https://github.com/pichi-router/pichi/actions/workflows/windows.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/windows.yml) | [![Android](https://github.com/pichi-router/pichi/actions/workflows/android.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/android.yml) | [![iOS](https://github.com/pichi-router/pichi/actions/workflows/ios.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/ios.yml) |

# Supported protocols

| Protocol | Ingress | Egress | Description |
|:---|:---:|:---:|:---|
| **Direct** | ❌ | ✅ | connecting to destination directly |
| **DUAL** | ✅ | ❌ | HTTP & SOCKS5 |
| **HTTP** | ✅ | ✅ | defined by [RFC 2068](https://www.ietf.org/rfc/rfc2068.txt) and [RFC 2616](https://www.ietf.org/rfc/rfc2817.txt) |
| **Reject** | ❌ | ✅ | rejecting request immediately or after a fixed/random delay |
| **Shadowsocks** | ✅ | ✅ | defined by [shadowsocks.org](https://shadowsocks.org/en/spec/Protocol.html) |
| **SOCKS5** | ✅ | ✅ | defined by [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt) |
| **Transparent** | ✅ | ❌ | Transparent proxy for TCP |
| **Trojan** | ✅ | ✅ | defined by [trojan-gfw](https://trojan-gfw.github.io/trojan/protocol) and its websocket extension defined by [trojan-go](https://github.com/p4gefau1t/trojan-go) |
| **Tunnel** | ✅ | ❌ | TCP tunnel to multiple destinations to be chosen by pre-defined load balance |

# Donation

If you enjoy using Pichi, consider buying me a coffee.

<img src="docs/assets/images/tip.png" alt="Please donate BTC" width="300"/>

