FROM debian:trixie
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc make binutils ca-certificates python3 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /chal
COPY . /chal
RUN make clean all
CMD ["./chall"]
