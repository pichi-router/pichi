---
title: Home
nav_order: 1
description: "A flexible rule-based proxy."
permalink: /
---

*Pichi* is a flexible rule-based proxy.
{: .fs-6 .fw-300 }

[Get started](/getting-started){: .btn .btn-primary .fs-5 .mb-4 .mb-md-0 .mr-2 }
[Download](https://github.com/pichi-router/pichi/releases){: .btn .fs-5 .mb-4 .mb-md-0 }

# Supported protocols

## Ingress protocols

| Protocol | Descritpion |
|:---|:---|
| **HTTP Proxy** | defined by [RFC 2068](https://www.ietf.org/rfc/rfc2068.txt) |
| **HTTP Tunnel** | defined by [RFC 2616](https://www.ietf.org/rfc/rfc2817.txt) |
| **SOCKS5** | defined by [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt) |
| **DUAL** | HTTP & SOCKS5 |
| **Shadowsocks** | defined by [shadowsocks.org](https://shadowsocks.org/en/spec/Protocol.html) |
| **Trojan** | defined by [trojan-gfw](https://trojan-gfw.github.io/trojan/protocol) and its websocket extension defined by [trojan-go](https://github.com/p4gefau1t/trojan-go) |
| **Tunnel** | TCP tunnel to multiple destinations to be chosen by pre-defined load balance |
| **Transparent** | Transparent proxy for TCP |

## Egress protocols

| Protocol | Descritpion |
|:---|:---|
| **Direct** | connecting to destination directly |
| **Reject** | rejecting request immediately or after a fixed/random delay |
| **HTTP Proxy** | defined by [RFC 2068](https://www.ietf.org/rfc/rfc2068.txt) |
| **HTTP Tunnel** | defined by [RFC 2616](https://www.ietf.org/rfc/rfc2817.txt) |
| **SOCKS5** | defined by [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt) |
| **Shadowsocks** | defined by [shadowsocks.org](https://shadowsocks.org/en/spec/Protocol.html) |
| **Trojan** | defined by [trojan-gfw](https://trojan-gfw.github.io/trojan/protocol) and its websocket extension defined by [trojan-go](https://github.com/p4gefau1t/trojan-go) |

**NOTE:** HTTP egress always attempts an [HTTP CONNECT](https://www.ietf.org/rfc/rfc2817.txt) handshake first. If the handshake fails, an [HTTP Proxy](https://www.ietf.org/rfc/rfc2068.txt).

# Build Status

## Server/Desktop

| OS | Ubuntu 24.04 | macOS 26 | Windows Server 2025 |
|:---:|:---:|:---:|:---:|
| **Toolchain** | GCC 13.3.0 | Xcode 26.4.1 | Visual Studio 2022 |
| **Status** | [![Linux](https://github.com/pichi-router/pichi/actions/workflows/linux.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/linux.yml) | [![macOS](https://github.com/pichi-router/pichi/actions/workflows/macos.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/macos.yml) | [![Windows](https://github.com/pichi-router/pichi/actions/workflows/windows.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/windows.yml) |

## Mobile

| OS | Android | iOS |
|:---:|:---:|:---:|
| **Toolchain** | Android NDK r29 | Xcode 26.4.1 |
| **Status** | [![Android](https://github.com/pichi-router/pichi/actions/workflows/android.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/android.yml) | [![iOS](https://github.com/pichi-router/pichi/actions/workflows/ios.yml/badge.svg?branch=main)](https://github.com/pichi-router/pichi/actions/workflows/ios.yml) |

# Donation

If you enjoy using Pichi, consider buying me a coffee.

<img src="/assets/images/tip.png" alt="Please donate BTC" width="300"/>
