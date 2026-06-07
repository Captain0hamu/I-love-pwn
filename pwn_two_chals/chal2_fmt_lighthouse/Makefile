CC := gcc
CFLAGS := -O0 -g -fno-stack-protector -no-pie -Wall -Wextra
LDFLAGS := -Wl,-z,relro -Wl,-z,lazy -Wl,--dynamic-linker=./ld-linux-x86-64.so.2 -Wl,-rpath,'$$ORIGIN'

BIN := chall
SRC := chal.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(BIN)

checksec:
	@echo "Use: checksec --file=./$(BIN)"
