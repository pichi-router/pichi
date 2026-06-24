---
title: Getting Started
nav_order: 2
has_toc: false
permalink: /getting-started
---

# Installation

## Pre-built binaries

Download pre-built binaries from [Releases](https://github.com/pichi-router/pichi/releases) page.
All the pre-built binaries enable following features:

| Feature | Operating Systems |
|:---|:---|
| **TLS_FINGERPRINT** | ALL |
| **TRANSPARENT_PF** | FreeBSD, macOS |
| **TRANSPARENT_IPTABLES** | Linux |

## Docker

The pre-built Docker image is available on [GitHub Package](https://github.com/pichi-router/pichi/pkgs/container/pichi),
which is automatically generated according to `docker/pichi.dockerfile`.

```
$ docker pull ghcr.io/pichi-router/pichi
$ docker run -d --name pichi --net host --restart always ghcr.io/pichi-router/pichi \
>   pichi -g /usr/share/pichi/geo.mmdb -p 1024 -l 127.0.0.1
c51b832bd29dd0333b0d32b0b0563ddc72821f7301c36c7635ae47d00a3bb902
$ docker ps -n 1
CONTAINER ID        IMAGE                              COMMAND                  CREATED             STATUS              PORTS               NAMES
c51b832bd29d        ghcr.io/pichi-router/pichi         "pichi -g /usr/share…"   1 seconds ago       Up 1 seconds                            pichi
```

{: .note }
The pre-built image enables `TLS_FINGERPRINT` and `TRANSPARENT_IPTABLES` feature.

## Homebrew

{: .warning }
[Homebrew Tap for Pichi](https://github.com/pichi-router/homebrew-pichi) repository is deprecated since [1.5.0](https://github.com/pichi-router/pichi/releases/tag/1.5.0).

## Others

Please refer to [Build](/getting-started/build) page to build from scratch.
