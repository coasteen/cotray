CXX = g++
CXXFLAGS = -Wall -O2
LDFLAGS = -lX11
PREFIX = /usr

all: cotray

cotray: cotray.cpp
	$(CXX) $(CXXFLAGS) -o cotray cotray.cpp $(LDFLAGS)

install: cotray cotray.1
	sudo install -Dm755 cotray $(PREFIX)/bin/cotray
	sudo install -Dm644 cotray.1 $(PREFIX)/share/man/man1/cotray.1

clean:
	rm -f cotray
