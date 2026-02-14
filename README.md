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


## Dev

running with cgi cache bust
```
docker build --build-arg CGI_CACHE_BUST=$(date +%s) -t webserv .
```
