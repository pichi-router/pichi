---
title: API Specification
nav_order: 4
has_toc: false
permalink: /api-specification
---

# API specification

Pichi listens on a local endpoint (for example `[::1]:21127`) to accept HTTP requests for querying or modifying its behavior.
All API communication adheres to REST principles.
Before breaking down the detailed API specification, it is helpful to establish the core resource concepts available to users.

| Resource | Path | Description |
|:---|:---|:---|
| [Ingress](ingress) | /ingresses/{name} | An ingress defines an incoming network adapter, specifying its protocol type, listening address/port, and protocol-specific configurations. |
| [Egress](egress) | /egresses/{name} | An egress defines an outgoing network adapter, specifying its protocol type, address/port of next hop, and protocol-specific configurations. |
| [Rule](rule) | /rules/{name} | A rule consists of a set of conditions, such as IP ranges, domain regular expressions, or destination countries. An incoming connection matches the rule if it satisfies **ANY** of these conditions. |
| [Route](route) | /route | Route defines a priority-ordered sequence of `[rule0, rule1, ..., egress]` tuples, along with a `default` egress used if none of the rules match. |
