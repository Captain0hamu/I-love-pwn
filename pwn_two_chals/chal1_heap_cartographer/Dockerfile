FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive
ENV USER=pwn
ENV HOME=/home/pwn
ENV WORKDIR=/work

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc \
    g++ \
    make \
    gdb \
    python3 \
    python3-pip \
    python3-venv \
    git \
    curl \
    wget \
    ca-certificates \
    file \
    less \
    vim \
    nano \
    tmux \
    strace \
    ltrace \
    patchelf \
    procps \
    netcat-openbsd \
    socat \
    xxd \
    binutils \
    libc6-dbg \
    sudo \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash "${USER}" \
    && echo "${USER}:${USER}" | chpasswd \
    && usermod -aG sudo "${USER}" \
    && echo "${USER} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/pwn \
    && chmod 0440 /etc/sudoers.d/pwn

# pwntools
RUN python3 -m pip install --break-system-packages --no-cache-dir \
    pwntools \
    ropper \
    one_gadget

# GEF
RUN wget -O /home/${USER}/.gef.py https://gef.blah.cat/py \
    && chown ${USER}:${USER} /home/${USER}/.gef.py

# gdbinit
RUN printf '%s\n' \
    'source ~/.gef.py' \
    'set disassembly-flavor intel' \
    'set pagination off' \
    'set confirm off' \
    'set print pretty on' \
    > /home/${USER}/.gdbinit \
    && chown ${USER}:${USER} /home/${USER}/.gdbinit

# bashrc: host側のbashrcがmountされていたら読む
RUN cat > /home/${USER}/.bashrc <<'EOF'
# ~/.bashrc inside pwn container

export TERM=xterm-256color
export EDITOR=vim
export PWNDBG_NO_AUTOUPDATE=1
export HISTFILE=/home/pwn/.bash_history
export HISTSIZE=10000
export HISTFILESIZE=20000

alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'
alias gdb='gdb -q'
alias py='python3'
alias pwninit='echo "use ./libc.so.6 and ./ld-linux-x86-64.so.2 manually or with patchelf"'

# Optional: source host bashrc if mounted by compose.
# Keep it after basic aliases, so your host settings can override them.
if [ -f /home/pwn/.host_bashrc ]; then
    source /home/pwn/.host_bashrc
fi

cd /work 2>/dev/null || true
EOF

RUN chown ${USER}:${USER} /home/${USER}/.bashrc

WORKDIR /work
USER ${USER}

CMD ["/bin/bash"]
