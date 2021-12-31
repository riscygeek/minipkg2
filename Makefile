
CC ?= gcc
CFLAGS ?= -Wall -Wextra -Og -g -Iinclude

sources = $(wildcard src/*.c)
objects = $(patsubst src/%.c,obj/%.o,$(sources))

all: minipkg2

minipkg2: $(objects)
	$(CC) -o $@ $(objects) $(LDFLAGS)

obj/%.o: src/%.c include
	@mkdir -p obj
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf obj
	rm -f minipkg2

.PHONY: all clean
