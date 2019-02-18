BIN=unit_test
CC=gcc
CFLAGS=-I.
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=%.o)

all: $(BIN)

%.o: %.c $(SRCS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o $(BIN)
