CC := gcc
TARGET := pktRxTx
SRC := $(wildcard ./Source/*.c)
CFLAGS := -g -O0
LFLAGS := -IInclude -lpthread -lpcap #/usr/lib64/libpcap.a -lnet 

.PHONY: all
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

.PHONY: clean
clean:
	rm -f ./Source/*.o $(TARGET)
