all: config.h badbar

CC=gcc

LIBS=-lX11
CFLAGS=

FLAGS=-Os -pipe -s -pedantic
DEBUGFLAGS=-Og -pipe -g -Wall -pedantic

INPUT=badbar.c
OUTPUT=badbar

INSTALLDIR=$(HOME)/.local/bin

RM=/bin/rm
CP=/bin/cp

.PHONY: badbar
badbar:
	$(CC) $(INPUT) -o $(OUTPUT) $(LIBS) $(CFLAGS) $(FLAGS)

debug:
	$(CC) $(INPUT) -o $(OUTPUT) $(LIBS) $(CFLAGS) $(DEBUGFLAGS)

test:
	./$(OUTPUT)

install:
	test -d $(INSTALLDIR) || mkdir -p $(INSTALLDIR)
	install -pm 755 $(OUTPUT) $(INSTALLDIR)

uninstall:
	$(RM) $(INSTALLDIR)/$(OUTPUT)

config.h:
	if [ ! -e "config.h" ]; then $(CP) "config.def.h" "config.h"; fi

clean:
	if [ -e $(OUTPUT) ]; then $(RM) $(OUTPUT); fi
