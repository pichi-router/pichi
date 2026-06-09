---
title: Usage
nav_order: 3
parent: Getting Started
permalink: /getting-started/usage
---

# Usage

## Command line options

```bash
$ pichi -h
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

{: .note }
> The `--port` option is mandatory.

The `--config` option specifies the initial configuration file complying with [Configuration](./configuration).
If omitted, Pichi defaults to the following configuration:

```js
{
  "ingresses": { /* No ingress */ },
  "rules": { /* No rule */ },
  "egresses": { "direct": {"type": "direct"} },
  "route": {"default": "direct"}
}
```

## Reload configuration

Following POSIX conventions, Pichi reloads the configuration file specified by the `--config` option
upon receiving a `SIGHUP` signal, regardless of whether the process is daemonized.

```bash
# Terminal 1                                      | # Terminal 2
$ pichi -p 21127 -c /path/to/config.json          | $
Loading configuration: /path/to/config.json       | $
Configuration /path/to/config.json loaded         | $
...                                               | $
...                                               | $
...                                               | $
...                                               | $ pkill -HUP pichi
Configuration reset                               | $
Loading configuration: /path/to/config.json       | $
Configuration /path/to/config.json loaded         | $
...                                               | $
...                                               | $
```
