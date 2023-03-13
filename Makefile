CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -Werror -pedantic -std=c99
LDFLAGS ?= -lpthread -lvncserver

SOURCEDIR = src
BUILDDIR = build
BINDIR = bin

EXECUTABLE = sakurakvmd
SOURCES = $(shell find $(SOURCEDIR) -name '*.c')
OBJECTS = $(patsubst $(SOURCEDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

all: $(BINDIR)/$(EXECUTABLE)

$(BINDIR)/$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)/*
	rm -rf $(BINDIR)/*
