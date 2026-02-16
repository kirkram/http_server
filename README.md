# WebServ

WebServ is a simple web server implemented in C++. It supports basic HTTP functionalities including GET, POST, and DELETE requests, directory listing, and CGI execution.
Built for Linux environments.

## Installation

1. Clone the repository:

   ```sh
   git clone https://github.com/kirkram/http_server.git
   cd http_server
   ```

2. Build the project:
   ```sh
   make
   ```

## Usage

For Linux users running outside Docker, create the temp directory before launching:

```sh
mkdir -p /tmp/webserv
```

To start the server, run the following command:

```sh
./webserv [configuration file]
```

If no configuration file is provided, the server will use the default configuration.

## Docker

Build the image:

```sh
docker build -t webserv .
```

Build with debug logs enabled (`-DDEBUG`):

```sh
docker build --build-arg BUILD_MODE=debug -t webserv:debug .
```

Run attached so `Ctrl+C` stops the container:

```sh
docker run --init --rm -p 8080:8080 -p 8081:8081 --name webserv webserv
```

## Configuration

Configuration files are located in the conf/ directory. You can specify a custom configuration file when starting the server.

## Features

- GET Requests: Serve static files and directory listings.
- POST Requests: Handle form submissions and file uploads.
- DELETE Requests: Delete files from the server.
- CGI Support: Execute CGI scripts for dynamic content.

## Project Requirements (Summary)

### Core server behavior

- Accept a config file path as CLI argument, with a fallback default config.
- Do not launch or proxy to another web server binary.
- Keep the server event-driven and non-blocking.
- Use one polling mechanism (`poll` or equivalent) for all socket I/O, including listening sockets.
- Watch readable and writable states in the same event loop.
- Only perform network `read/recv` and `write/send` when the poller indicates readiness.
- Ensure no request can hang forever (timeouts and cleanup are required).
- Return correct HTTP status codes and provide default error pages when custom ones are not configured.
- Support browser usage and compare behavior with NGINX when semantics are unclear.
- Use `fork` only for CGI execution.
- Serve static content, file uploads, and at least `GET`, `POST`, and `DELETE`.
- Stay resilient under stress/load; server availability is a core requirement.
- Support multiple listening ports via configuration.

### Configuration file expectations

- Define host and port per server block.
- Optionally define `server_name`.
- For the same host:port, the first declared server acts as default.
- Configure custom error pages and max client body size.
- Configure routes without regex, including:
  accepted methods, redirects, root mapping, autoindex on/off, default index file, CGI by extension, and upload destination.
- Routes with CGI must work with `GET` and `POST`.
- For CGI handling:
  use full `PATH_INFO`, unchunk request bodies before passing to CGI, use EOF as body end when needed, pass requested script as first argument, and run CGI from the correct working directory for relative paths.
- Provide enough sample config/static files to demonstrate features during evaluation.


## Dev

running with index.html cache bust
```
docker build --build-arg WWW_CACHE_BUST=$(date +%s) -t webserv .
```
or bust CGI and index.html both
```
docker build --build-arg CGI_CACHE_BUST=$(date +%s) --build-arg WWW_CACHE_BUST=$(date +%s) -t webserv .
```

stress testing helpers (Go + Node baseline) are available in:
`stress/README.md`


For online GCP Docker
```
docker buildx build --platform linux/amd64 -t europe-north1-docker.pkg.dev/nomadic-genre-456612-t9/my-repo/my-app:latest .
```
```
docker push europe-north1-docker.pkg.dev/nomadic-genre-456612-t9/my-repo/my-app:latest 
```