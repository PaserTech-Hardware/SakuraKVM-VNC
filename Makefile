CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -Werror -pedantic -std=c99
LDFLAGS ?= -lpthread -lvncserver-sakura

SOURCEDIR = src
BUILDDIR = build
BINDIR = bin

GITVER := $(shell git describe --match=NeVeRmAtCh --always --abbrev=8 --dirty)
ifneq ($(GITVER),)
CFLAGS += -DSAKURAKVMD_GIT_VER=\"$(GITVER)\"
endif

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
