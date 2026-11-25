# Quick build without CMake:   make        →  ./cactulator
#                              make test   →  runs the core tests
# (CMake works too: cmake -B build && cmake --build build)

CXX      ?= c++
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++17 -Wall -Wextra -Isrc

UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
  CURSES_LIBS ?= -lncurses
else
  CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null || echo -lncursesw)
endif

BUILD    := build-make
CORE_SRC := $(wildcard src/core/*.cpp)
APP_SRC  := $(wildcard src/ui/*.cpp) src/main.cpp src/repl.cpp
CORE_OBJ := $(CORE_SRC:%.cpp=$(BUILD)/%.o)
APP_OBJ  := $(APP_SRC:%.cpp=$(BUILD)/%.o)
DEPS     := $(CORE_OBJ:.o=.d) $(APP_OBJ:.o=.d) $(BUILD)/tests/test_core.d

all: cactulator

cactulator: $(CORE_OBJ) $(APP_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CURSES_LIBS)

$(BUILD)/test_core: $(BUILD)/tests/test_core.o $(CORE_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(BUILD)/test_core
	cd $(BUILD) && ./test_core

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

install: cactulator
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 cactulator $(DESTDIR)$(PREFIX)/bin/cactulator

PREFIX ?= /usr/local

clean:
	rm -rf $(BUILD) cactulator

.PHONY: all test clean install

-include $(DEPS)
