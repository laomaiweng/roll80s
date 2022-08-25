BIN = ars2000
SRC = main.c
HDR = banner.h
override CFLAGS += -Wall -Wextra -std=gnu11

release: CFLAGS += -O3
release: $(BIN)

debug: CFLAGS += -O0 -ggdb3 -DVERBOSE -fsanitize=address
debug: $(BIN)

afl: CC=afl-cc
afl: BIN=ars2000-afl
afl: release

$(BIN): $(SRC) $(HDR)
	$(CC) -o $(BIN) $(CFLAGS) $(LDFLAGS) $(SRC)

clean:
	$(RM) $(BIN)

.PHONY: release debug clean
