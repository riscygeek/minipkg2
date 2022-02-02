VERSION = 0.1.6


# Compiler definitions.
CC 		?= gcc
CFLAGS 	+= -std=gnu11 -Wall -Wextra -Og -g -Iinclude

# Install directories.
prefix	?= /usr/local
bindir	?= bin

CPPFLAGS += -DVERSION=\"$(VERSION)\"

# Enable/disable support for downloading packaes with libcurl.
ifeq ($(HAS_LIBCURL),1)
CPPFLAGS += -DHAS_LIBCURL=1
LIBS 		+= -lcurl
endif

sources = $(wildcard src/*.c)
objects = $(patsubst src/%.c,obj/%.o,$(sources))

all: minipkg2

minipkg2: $(objects)
	$(CC) -o $@ $(objects) $(LDFLAGS) $(LIBS)

obj/%.o: src/%.c include Makefile
	@mkdir -p obj
	$(CC) -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

install:
	install -vDm755 minipkg2 $(DESTDIR)$(prefix)/$(bindir)/minipkg2

clean:
	rm -rf obj
	rm -f minipkg2

.PHONY: all install clean
