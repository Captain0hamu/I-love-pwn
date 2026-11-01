# vuln_notepad — build and package
CC       ?= gcc
CFLAGS   ?= -O0 -Wall -Wextra
HARDENED := -fstack-protector-all -pie -fpie \
            -Wl,-z,noexecstack,-z,now,-z,relro

.PHONY: all run package dist clean

all: vuln_notepad

vuln_notepad: vuln_notepad.c
	$(CC) $(CFLAGS) $(HARDENED) -o $@ $<

run: vuln_notepad
	./vuln_notepad

package:
	python3 package_dist.py

# Build in a Debian 13 container, then create the player archive.
dist:
	./build_dist.sh
	python3 package_dist.py

clean:
	rm -f vuln_notepad easy
	rm -rf build dist
