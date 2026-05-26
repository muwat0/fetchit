APP := fetchit
SRC := main.cpp
BUILD_DIR := build
DIST_DIR := dist

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -std=c++17 -pipe
LDFLAGS ?= -static-libstdc++ -static-libgcc

VERSION ?= $(shell git describe --tags --dirty --always 2>/dev/null)

.PHONY: all build run install uninstall clean dist release

all: build

build: | $(BUILD_DIR)
build: $(BUILD_DIR)/$(APP)

$(BUILD_DIR)/$(APP): $(SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/$(APP)

install: | $(BUILD_DIR)
install: build
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 $(BUILD_DIR)/$(APP) "$(DESTDIR)$(BINDIR)/$(APP)"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/$(APP)"

clean:
	rm -f $(BUILD_DIR)/$(APP)

dist: | $(BUILD_DIR) $(DIST_DIR)
dist: build
	tar -C $(BUILD_DIR) -czf $(DIST_DIR)/$(APP)-$(VERSION)-linux-x86_64-gnu.tar.gz $(APP)

$(DIST_DIR):
	mkdir -p $(DIST_DIR)

release: clean build dist
