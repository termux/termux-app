PREFIX  ?= /data/data/com.termux/files/usr
CC      ?= cc
CFLAGS  ?= -O2
USB_CFLAGS := $(shell pkg-config --cflags libusb-1.0)
USB_LIBS   := $(shell pkg-config --libs libusb-1.0)

all: fido-bridge

fido-bridge: fido-bridge.c
	$(CC) -Wall -Wextra -std=gnu11 $(USB_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(USB_LIBS) -lpthread

install: fido-bridge
	install -Dm755 fido-bridge $(DESTDIR)$(PREFIX)/bin/fido-bridge
	install -Dm755 fido-ping.py $(DESTDIR)$(PREFIX)/bin/fido-ping

clean:
	rm -f fido-bridge

.PHONY: all install clean
