PREFIX=m68k-atari-mint
#CPP=$(PREFIX)-g++
CPP=$(PREFIX)-g++-4.6.4

CFLAGS=-std=gnu++0x -Wall -Wno-unused-value -Os -mshort -mfastcall #-mpcrel

LIBCMINI_DIR=$(HOME)/src/libcmini/build
LINK=-nostdlib $(LIBCMINI_DIR)/crt0.o $< -L$(LIBCMINI_DIR)/mshort/mfastcall $(LDFLAGS) -lcmini -lgcc -o $@
#LINK=$< $(LDFLAGS) -o $@

TARGETS=nighthax.prg

src = $(wildcard *.cc)
obj = $(src:.cc=.o)

.PHONY: all clean
all: $(TARGETS)
clean:
	rm -f $(TARGETS) $(obj)

nighthax.prg: nighthax.cc
	$(CPP) $(CFLAGS) $(LINK)
	$(PREFIX)-objdump -drwC $@ > $@.s
	$(PREFIX)-strip --strip-unneeded -R .comment -R .note -R .note.ABI-tag $@
	$(PREFIX)-size $@
