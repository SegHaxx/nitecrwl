CC=m68k-atari-mint-gcc
CFLAGS=-std=gnu99 -Wall -Wno-unused-value -Os
LDFLAGS=-Wl,--traditional-format -lgem

LIBCMINI_DIR=../libcmini/build
LINK=-nostdlib $(LIBCMINI_DIR)/crt0.o $< -L$(LIBCMINI_DIR) $(LDFLAGS) -lcmini -lgcc -o $@
#LINK=$< $(LDFLAGS) -o $@

all: nighthax.prg

nighthax.prg: nighthax.c Makefile
	$(CC) $(CFLAGS) -march=68000 -mtune=68020-40 $(LINK)

.PHONY: clean

clean:
	rm -f nighthax.prg
