LIBS := $(shell pkg-config --libs glib-2.0 gobject-2.0 libsoup-2.2)
CFLAGS := $(shell pkg-config --cflags glib-2.0 gobject-2.0 libsoup-2.2)

CC := gcc -c -g -Wall
LINK := gcc

all: soup-giochannel-experiment

soup-giochannel-experiment: soup-giochannel-experiment.o
	$(LINK) $(LIBS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $<

test-file.orig:
	dd if=/dev/urandom of=$@ count=10378 bs=1k

run: soup-giochannel-experiment test-file.orig
	cat test-file.orig | ./soup-giochannel-experiment &
	curl http://localhost:3333/test > test-file
	cmp test-file.orig test-file

.PHONY: run
