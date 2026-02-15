FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        make \
        python3 \
    && rm -rf /var/lib/apt/lists/*

COPY Makefile ./
COPY includes ./includes
COPY srcs ./srcs
COPY conf ./conf

ARG BUILD_MODE=release
RUN if [ "$BUILD_MODE" = "debug" ]; then make debug; else make; fi \
    && mkdir -p uploads \
    && mkdir -p /tmp/webserv

ARG CGI_CACHE_BUST=0
COPY cgi-bin ./cgi-bin
RUN echo "CGI cache bust: ${CGI_CACHE_BUST}" > /tmp/cgi_cache_bust \
    && c++ -std=c++20 -O2 cgi-bin/cgi.cpp -o cgi-bin/print_form.cgi

ARG WWW_CACHE_BUST=0
COPY www ./www
RUN echo "WWW cache bust: ${WWW_CACHE_BUST}" > /tmp/www_cache_bust

EXPOSE 8080 8081

CMD ["sh", "-c", "sed 's/127\\.0\\.0\\.1:/0.0.0.0:/g' conf/default.conf > /tmp/default.conf && exec ./webserv /tmp/default.conf"]
