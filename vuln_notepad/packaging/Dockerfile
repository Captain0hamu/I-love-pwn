FROM debian:trixie-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY vuln_notepad.c ./

RUN mkdir -p /out \
    && gcc -O0 -Wall -Wextra \
        -fstack-protector-all -pie -fpie \
        -Wl,-z,noexecstack,-z,now,-z,relro \
        -o /out/vuln_notepad vuln_notepad.c \
    && cp /lib/x86_64-linux-gnu/libc.so.6 /out/libc.so.6 \
    && cp -L /lib64/ld-linux-x86-64.so.2 /out/ld-linux-x86-64.so.2
