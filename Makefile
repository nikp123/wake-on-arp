src = $(wildcard src/*.c)
obj = $(src:.c=.o)

CFLAGS = -I./include -O2
LDFLAGS = 

wake-on-arp: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) wake-on-arp

