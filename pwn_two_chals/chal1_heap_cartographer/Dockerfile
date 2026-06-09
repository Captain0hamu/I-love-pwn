FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive
ENV TERM=xterm-256color

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    make \
    binutils \
    libc6-dbg \
    gdb \
    gdbserver \
    python3 \
    python3-pip \
    python3-pwntools \
    git \
    curl \
    ca-certificates \
    file \
    patchelf \
    procps \
    strace \
    ltrace \
    netcat-openbsd \
    vim-tiny \
    tmux \
    less \
    && rm -rf /var/lib/apt/lists/*

# GEF for gdb.  Mounted challenge files live in /work via compose.yaml.
RUN git clone --depth 1 https://github.com/hugsy/gef.git /opt/gef \
    && printf 'source /opt/gef/gef.py\nset disassembly-flavor intel\nset pagination off\n' > /root/.gdbinit

WORKDIR /work

CMD ["/bin/bash"]

