BIN = roll80s
SRC = main.c
CFLAGS = -Wall -Wextra -std=gnu11

release: CFLAGS += -O3
release: $(BIN)

debug: CFLAGS += -O0 -ggdb3 -DVERBOSE
debug: $(BIN)

$(BIN): $(SRC)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(SRC)

clean:
	$(RM) $(BIN)

.PHONY: release debug clean
