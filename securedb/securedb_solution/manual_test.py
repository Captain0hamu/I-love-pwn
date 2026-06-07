#!/usr/bin/env python3
import subprocess

proc = subprocess.Popen(['./securedb'], 
                       stdin=subprocess.PIPE, 
                       stdout=subprocess.PIPE, 
                       stderr=subprocess.PIPE,
                       text=True)

# Commands
cmds = [
    "1", "large_libc", "1280",  # new large record
    "1", "fence", "16",          # new fence record
    "2", "0",                    # delete large record
    "5", "0",                    # read deleted record
    "0"                          # quit
]

for cmd in cmds:
    proc.stdin.write(cmd + '\n')
    proc.stdin.flush()

stdout, stderr = proc.communicate()
print("=== Output ===")
print(stdout[-1000:])  # Last 1000 chars
