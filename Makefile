src = $(wildcard src/*.c)
obj = $(src:.c=.o)

PREFIX ?= /usr/local
CONFIG_PREFIX ?= /etc

CFLAGS = -I./include -DCONFIG_PREFIX=\"$(CONFIG_PREFIX)\"
LDFLAGS = -O2

all: wake-on-arp

debug: LDFLAGS = -g
debug: CFLAGS += -g
debug: wake-on-arp

wake-on-arp: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

install:
	install -Dm755 wake-on-arp $(PREFIX)/bin/wake-on-arp
	test -f $(CONFIG_PREFIX)/wake-on-arp.conf || install -Dm600 example.conf $(CONFIG_PREFIX)/wake-on-arp.conf

.PHONY: clean
clean:
	rm -f $(obj) wake-on-arp

