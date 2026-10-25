PREFIX  ?= /data/data/com.termux/files/usr
CC      ?= cc
CFLAGS  ?= -O2
USB_CFLAGS := $(shell pkg-config --cflags libusb-1.0)
USB_LIBS   := $(shell pkg-config --libs libusb-1.0)
# Where python3 looks for modules, so scripts can "import termux_fido".
PYSITE  ?= $(shell python3 -c 'import sysconfig; print(sysconfig.get_path("purelib"))' 2>/dev/null)
BIN      = $(DESTDIR)$(PREFIX)/bin
SHORTCUT = $(HOME)/.shortcuts/fido-up

all: fido-bridge

fido-bridge: fido-bridge.c
	$(CC) -Wall -Wextra -std=gnu11 $(USB_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(USB_LIBS) -lpthread

install: fido-bridge
	install -Dm755 fido-bridge $(BIN)/fido-bridge
	install -Dm755 fido-ping.py $(BIN)/fido-ping
	install -Dm755 fido-info.py $(BIN)/fido-info
	# Write then rename: "fido update" runs this while bash is still reading $(BIN)/fido.
	sed 's|@SRCDIR@|$(CURDIR)|' fido.sh > $(BIN)/.fido.tmp && chmod 755 $(BIN)/.fido.tmp && mv -f $(BIN)/.fido.tmp $(BIN)/fido
ifneq ($(PYSITE),)
	install -Dm644 termux_fido.py $(DESTDIR)$(PYSITE)/termux_fido.py
else
	@echo "python3 not found: skipped installing termux_fido.py"
endif

# One-tap start from the home screen with the Termux:Widget app.
widget:
	mkdir -p "$(dir $(SHORTCUT))"
	printf '#!/data/data/com.termux/files/usr/bin/bash\nfido up\nsleep 2\n' > "$(SHORTCUT)"
	chmod 755 "$(SHORTCUT)"
	@echo "Added a fido-up shortcut; add the Termux:Widget widget to your home screen."

clean:
	rm -f fido-bridge

.PHONY: all install widget clean
