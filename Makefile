BIN = roll80s
SRC = main.c
override CFLAGS += -Wall -Wextra -std=gnu11

release: CFLAGS += -O3
release: $(BIN)

debug: CFLAGS += -O0 -ggdb3 -DVERBOSE
debug: $(BIN)

afl: CC=afl-cc
afl: BIN=roll80s-afl
afl: release

$(BIN): $(SRC)
	$(CC) -o $(BIN) $(CFLAGS) $(LDFLAGS) $(SRC)

clean:
	$(RM) $(BIN)

.PHONY: release debug clean
