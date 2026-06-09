---
title: Configuration
nav_order: 2
parent: Getting Started
permalink: /getting-started/configuration
---

# Structure

Pichi accepts a **JSON** configuration file structured as follows:

```js
{
  "ingresses": {
    "ingress-0": {/* ingress configuration */},
    "ingress-1": {/* ingress configuration */}
  },
  "egresses": {
    "egress-0": {/* egress configuration */},
    "egress-1": {/* egress configuration */}
  },
  "rules": {
    "rule-0": {/* rule configuration */},
    "rule-1": {/* rule configuration */}
  },
  "route": {/* route configuration */}
}
```

Please refer to [API Specification](../api-specification) for detailed section description.

# Examples

## Proxy like ss-local

```js
{
  "ingresses": {
    "s5": {
      "type": "socks5",
      "bind": [{"host": "127.0.0.1", "port": 1080}]
    }
  },
  "egresses": {
    "ss": {
      "type": "ss",
      "server": {"host": "my-ss-server", "port": 8388},
      "option": {"method": "aes-256-gcm", "password": "my-password"}
    }
  }
  "rules": { /* not necessary */ },
  "route": {"default": "ss"}
}
```

## HTTPS proxy except intranet

```js
{
  "ingresses": {
    "http": {
      "type": "http",
      "bind": [{"host": "127.0.0.1", "port": 8080}]
    }
  },
  "egresses": {
    "direct": {"type": "direct"},
    "http": {
      "type": "http",
      "server": {"host": "my-http-server", "port": 8080},
      "tls": {"insecure": false, "sni": "my-http-server"}
    }
  },
  "rules": {
    "intranet": {
      "domain": ["local"],
      "pattern": ["^localhost$"],
      "range": [
        "0.0.0.0/8", "10.0.0.0/8", "100.64.0.0/10", "127.0.0.0/8", "169.254.0.0/16",
        "172.16.0.0/12", "192.168.0.0/16", "198.18.0.0/16", "224.0.0.0/4",
        "::1/128", "fc00::/7", "fe80::/10", "fd00::/8"
      ]
    }
  },
  "route": {
    "default": "http",
    "rules": [
      ["intranet", "direct"]
    ]
  }
}
```

## Dark web

```js
{
  "ingresses": {
    "dual": {
      "type": "dual",
      "bind": [
        {"host": "127.0.0.1", "port": 1080},
        {"host": "127.0.0.1", "port": 8080},
      ]
    }
  },
  "egresses": {
    "direct": {"type": "direct"},
    "tor": {
      "type": "socks5",
      "server": {"host": "localhost", "port": 9050}
    },
    "i2p": {
      "type": "http",
      "server": {"host": "localhost", "port": 4444}
    }
  },
  "rules": {
    "onion": {"domain": [".onion"]},
    "i2p": {"domain": [".i2p"]}
  },
  "route": {
    "default": "direct",
    "rules": [
      ["onion", "tor"]
      ["i2p", "i2p"]
    ]
  }
}
```

## HTTP/Socks5 server with the certificate issued by Let's encrypt

```js
{
  "ingresses": {
    "dual": {
      "type": "dual",
      "bind": [{"host": "::", "port": 8118}],
      "tls": {
        "key_file": "/etc/letsencrypt/live/example.com/privkey.pem",
        "cert_file": "/etc/letsencrypt/live/example.com/fullchain.pem"
      }
    }
  },
  "egresses": {"direct": {"type": "direct"}},
  "route": {"default": "direct"}
}
```

## Balanced DNS-over-TLS proxy through Trojan

```js
{
  "ingresses": {
    "dot": {
      "type": "tunnel",
      "bind": [{"host": "::", "port": 853}],
      "option": {
        "balance": "random",
        "destinations": [
          { "host": "2606:4700:4700::1111", "port": 853 },
          { "host": "2606:4700:4700::1001", "port": 853 },
          { "host": "1.1.1.1", "port": 853 },
          { "host": "1.0.0.1", "port": 853 }
        ]
      }
    }
  },
  "egresses": {
    "trojan": {
      "type": "trojan",
      "server": {"host": "my-trojan-server", "port": 443},
      "tls": { "sni": "my-trojan-server" },
      "credential": {"password": "my-password"}
    }
  },
  "route": {"default": "trojan"}
}
```

## Transparent proxy through a Socks5s server

```js
{
  "ingresses": {
    "transparent": {
      "type": "transparent",
      "bind": [
        {"host":"::1", "port": 1001},
        {"host":"127.0.0.1", "port": 1001}
      ]
    }
  },
  "egresses": {
    "s5": {
      "type": "socks5",
      "server": {"host": "my-socks5s-server", "port": 1080},
      "tls": {"sni": "my-socks5-server"}
    }
  }
  /* "rules" node could be absent */
  "route": {"default": "s5"}
}
```

{: .note }
> Please make sure that Pichi is running with the correct privilege if you want to enable transparent ingress.

### Operating system supported by transparent ingress

| OS | Packet Filter |
|:---|:---|
| **\*BSD/macOS** | [PF](https://home.nuug.no/~peter/pf/en/) |
| **Linux** | [IPTABLES](https://man7.org/linux/man-pages/man8/iptables.8.html) |

### Packet filter's configuration

```bash
$ # Linux
$ sudo iptables -t nat -A PREROUTING -i eth0 -p tcp -j REDIRECT --to-ports 1001
$
$ # OpenBSD or FreeBSD 15.0
$ echo "pass in on fxp0 inet proto tcp from fxp0:network rdr-to 127.0.0.1 port 1001" | \
>   doas pfctl -a transparent_proxy -f -
$
$ # macOS or FreeBSD 14
$ echo "rdr pass on fxp0 inet proto tcp from fxp0:network -> 127.0.0.1 port 1001" | \
>   sudo pfctl -a transparent_proxy -f -
$
```
