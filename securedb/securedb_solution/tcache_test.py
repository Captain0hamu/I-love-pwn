#!/usr/bin/env python3
import subprocess

proc = subprocess.Popen(['./securedb'], 
                       stdin=subprocess.PIPE, 
                       stdout=subprocess.PIPE, 
                       stderr=subprocess.PIPE,
                       text=True)

# Test tcache chaining for heap leak
cmds = [
    "1", "h1", "40",  # new record 1
    "1", "h2", "40",  # new record 2  
    "2", "1",         # delete record 2
    "2", "0",         # delete record 1
    "5", "1",         # read record 1 (should contain pointer to record 2)
    "0"               # quit
]

for cmd in cmds:
    proc.stdin.write(cmd + '\n')
    proc.stdin.flush()

stdout, stderr = proc.communicate()
print("=== Tcache chaining test ===")
print(stdout[-1000:])
