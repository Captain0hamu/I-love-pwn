FROM debian:trixie-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends socat \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --uid 10001 --create-home --shell /bin/sh challenge

WORKDIR /challenge
COPY vuln_notepad libc.so.6 ld-linux-x86-64.so.2 run.sh ./
COPY flag.txt /flag
RUN chmod 0555 /challenge /challenge/vuln_notepad /challenge/ld-linux-x86-64.so.2 /challenge/run.sh \
    && chmod 0444 /challenge/libc.so.6 /flag

USER challenge
EXPOSE 9999
CMD ["socat", "TCP-LISTEN:9999,reuseaddr,fork", "EXEC:timeout 120 /challenge/run.sh,stderr"]
