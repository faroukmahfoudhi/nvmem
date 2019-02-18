BIN=unit_test
API=gpNvm
LIB=lib$(API)
CC=gcc
AR=ar
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=%.o)
CFLAGS=-I. -fPIC
LDFLAGS=-L. -lgpNvm

all: $(BIN) $(LIB).so 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB).a: $(API).o
	$(AR) rcs $@ $<
	
$(LIB).so: $(API).o
	$(CC) -shared -o $@ $< $(CFLAGS)

$(BIN): $(BIN).o $(LIB).a $(LIB).so
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o $(BIN) *.so *.a
