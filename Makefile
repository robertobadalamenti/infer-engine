CC = clang

# No -O flag on purpose: optimized code is reordered and inlined, which makes
# stepping through it misleading. Correctness first; add -O2/-O3 in the
# profiling phase, decided on measurements.
CFLAGS = -std=c11 -Wall -Wextra -Wimplicit-fallthrough -g

BIN = build/engine
SRC = src/main.c src/gguf.c
HDR = src/gguf.h

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRC)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf build

.PHONY: all run clean
