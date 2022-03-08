#CPP=m68k-atari-mint-g++-4.6.4
CPP=m68k-atari-mint-g++
CFLAGS=-std=gnu++0x -Wall -Wno-unused-value -Os #-mpcrel
LDFLAGS=-Wl,--traditional-format -lgem

LIBCMINI_DIR=/home/seg/src/libcmini/build
LINK=-nostdlib $(LIBCMINI_DIR)/crt0.o $< -L$(LIBCMINI_DIR) $(LDFLAGS) -lcmini -lgcc -o $@
#LINK=$< $(LDFLAGS) -o $@

all: nighthax.prg

nighthax.prg: nighthax.cc Makefile
	$(CPP) $(CFLAGS) -march=68000 -mtune=68020-40 $(LINK)
	m68k-atari-mint-strip --strip-unneeded -R .comment -R .note -R .note.ABI-tag $@

.PHONY: clean

clean:
	rm -f nighthax.prg
