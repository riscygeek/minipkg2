VERSION = 0.2.6


# Compiler definitions.
CC 		?= gcc
CFLAGS 	+= -std=gnu11 -Wall -Wextra -O2 -Iinclude

# Install directories.
prefix		?= /usr/local
bindir		?= bin
libdir		?= lib
sysconfdir	?= etc

CPPFLAGS += -DVERSION=\"$(VERSION)\"

# Enable/disable support for downloading packaes with libcurl.
ifeq ($(HAS_LIBCURL),1)
CPPFLAGS += -DHAS_LIBCURL=1
LIBS 		+= -lcurl
endif

CPPFLAGS += -DCONFIG_PREFIX=\"$(prefix)\"
CPPFLAGS += -DCONFIG_LIBDIR=\"$(libdir)\"
CPPFLAGS += -DCONFIG_SYSCONFDIR=\"$(sysconfdir)\"

sources = $(wildcard src/*.c)
objects = $(patsubst src/%.c,obj/%.o,$(sources))

all: minipkg2

minipkg2: $(objects)
	$(CC) -o $@ $(objects) $(LDFLAGS) $(LIBS)

obj/%.o: src/%.c include Makefile
	@mkdir -p obj
	$(CC) -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

install:
	install -vDm755 minipkg2			$(DESTDIR)$(prefix)/$(bindir)/minipkg2
	install -vDm644 util/minipkg2.conf	$(DESTDIR)$(prefix)/$(sysconfdir)/minipkg2.conf
	install -vDm644 util/env.bash		$(DESTDIR)$(prefix)/$(libdir)/minipkg2/env.bash

clean:
	rm -rf obj
	rm -f minipkg2

.PHONY: all install clean
