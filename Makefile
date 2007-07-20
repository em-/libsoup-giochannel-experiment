LIBS := $(shell pkg-config --libs glib-2.0 gobject-2.0 libsoup-2.2)
CFLAGS := $(shell pkg-config --cflags glib-2.0 gobject-2.0 libsoup-2.2)

CC := gcc -c
LINK := gcc

all: soup-giochannel-experiment

soup-giochannel-experiment: soup-giochannel-experiment.o
	$(LINK) $(LIBS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $<
