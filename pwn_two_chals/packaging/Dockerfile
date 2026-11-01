FROM debian:trixie-slim

ARG CHALLENGE

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY ${CHALLENGE}/chal.c ./chal.c

RUN mkdir -p /out \
    && gcc -O0 -g -Wall -Wextra -fno-stack-protector -no-pie \
        -Wl,-z,relro -Wl,-z,lazy \
        -Wl,--dynamic-linker=./ld-linux-x86-64.so.2 \
        -Wl,-rpath,'$ORIGIN' \
        -o /out/chall chal.c \
    && cp /lib/x86_64-linux-gnu/libc.so.6 /out/libc.so.6 \
    && cp -L /lib64/ld-linux-x86-64.so.2 /out/ld-linux-x86-64.so.2
