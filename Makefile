CXX = g++
CXXFLAGS = -Wall -O2
LDFLAGS = -lX11
PREFIX = /usr

all: cotray

PRIV_ESC := $(or $(shell command -v sudo 2>/dev/null), $(shell command -v doas 2>/dev/null), $(error "Error: Neither sudo nor doas found!! Install one!"))

cotray: cotray.cpp
	$(CXX) $(CXXFLAGS) -o cotray cotray.cpp $(LDFLAGS)

install: cotray cotray.1
	$(PRIV_ESC) install -Dm755 cotray $(PREFIX)/bin/cotray
	$(PRIV_ESC) install -Dm644 cotray.1 $(PREFIX)/share/man/man1/cotray.1

clean:
	rm -f cotray
