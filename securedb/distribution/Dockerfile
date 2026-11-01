FROM ubuntu:20.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends socat \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --uid 10001 --create-home --shell /bin/sh challenge

WORKDIR /challenge
COPY securedb libc.so.6 ld-2.31.so run.sh ./
COPY flag.txt /flag
RUN chmod 0555 /challenge /challenge/securedb /challenge/ld-2.31.so /challenge/run.sh \
    && chmod 0444 /challenge/libc.so.6 /flag

USER challenge
EXPOSE 9999
CMD ["socat", "TCP-LISTEN:9999,reuseaddr,fork", "EXEC:timeout 120 /challenge/run.sh,stderr"]
