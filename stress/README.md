# Stress Testing

This folder contains a reusable Go stress test tool and a simple Node.js baseline server for side-by-side comparisons.

## Files

- `stress/http_stress.go`: generic HTTP load generator (single target or compare mode).
- `stress/node_baseline_server.js`: lightweight Node server for baseline measurements.

## Quick Start

1. Start your web server under test.
2. (Optional) Start the Node baseline:

```sh
node stress/node_baseline_server.js
```

3. Run a single-target benchmark:

```sh
go run stress/http_stress.go \
  -url http://127.0.0.1:8080/ \
  -name webserv \
  -concurrency 100 \
  -duration 20s
```

Note: the Go stress tool does not parse HTML and fetch linked assets automatically.
It repeatedly calls only the URL you pass in `-url`.
If you want to stress asset paths too, run additional benchmarks for those endpoints
(for example `/img/background.jpg`, `/media/smiley.jpg`, `/video.html`).

4. Compare webserv vs Node:

```sh
go run stress/http_stress.go \
  -url http://127.0.0.1:8080/ -name webserv \
  -compare-url http://127.0.0.1:3000/ -compare-name node \
  -concurrency 100 \
  -duration 20s
```

## POST / upload-style test

Use `-method POST` and `-body-size` to stress request-body handling:

```sh
go run stress/http_stress.go \
  -url http://127.0.0.1:8080/ \
  -method POST \
  -body-size 1048576 \
  -header "Content-Type: application/octet-stream" \
  -concurrency 30 \
  -duration 15s
```

## Useful flags

- `-concurrency`: number of parallel workers.
- `-duration`: measured run duration.
- `-warmup`: warmup before measurement.
- `-timeout`: per-request timeout.
- `-header`: add custom request headers (repeatable).
- `-insecure`: skip TLS cert verification (for local HTTPS tests).

## Latest benchmark results

Environment:
- both servers running in Docker
- endpoint: `GET /`
- duration: `20s`
- concurrency: `100`

### webserv

- Operations: `248185`
- Responses: `248185`
- Transport errors: `0`
- Response read errors: `78`
- Bytes read: `1317596850`
- Req/sec: `12409.25`
- HTTP success rate: `100.00%`
- Status codes: `200=248185`
- Latency avg/p50/p90/p95/p99/max:
  `4.35424ms / 3.657792ms / 7.405167ms / 8.299709ms / 9.870792ms / 1.018370583s`

### node

- Operations: `174330`
- Responses: `174330`
- Transport errors: `0`
- Response read errors: `0`
- Bytes read: `925692300`
- Req/sec: `8716.50`
- HTTP success rate: `100.00%`
- Status codes: `200=174330`
- Latency avg/p50/p90/p95/p99/max:
  `11.453322ms / 11.548292ms / 12.35975ms / 13.091916ms / 16.016708ms / 135.469458ms`

### Comparison

- Req/sec: `webserv` better (`12409.25` vs `8716.50`)
- Success rate: tie (`100%`)
- Transport+read errors: `node` better (`0` vs `78`)
- p95 latency: `webserv` better (`8.299709ms` vs `13.091916ms`)
- p99 latency: `webserv` better (`9.870792ms` vs `16.016708ms`)
