# Copyright (C) 2026 StochasticCactus
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.14)
project(Cactulator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- curses (ncursesw on Linux/macOS/MSYS2, PDCurses elsewhere) -------------
set(CURSES_NEED_WIDE TRUE)
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses QUIET)
if(NOT CURSES_FOUND)
  # macOS ships wide-character support inside plain libncurses.
  unset(CURSES_NEED_WIDE)
  find_package(Curses REQUIRED)
endif()

set(CORE_SOURCES
  src/core/value.cpp
  src/core/format.cpp
  src/core/lexer.cpp
  src/core/parser.cpp
  src/core/interp.cpp
  src/core/builtins.cpp
  src/core/linalg.cpp
  src/core/numeric.cpp
  src/core/units.cpp
  src/core/rpn.cpp
  src/core/session.cpp
)

add_library(cactcore STATIC ${CORE_SOURCES})
target_include_directories(cactcore PUBLIC src)

add_executable(cactulator
  src/main.cpp
  src/repl.cpp
  src/ui/term.cpp
  src/ui/theme.cpp
  src/ui/canvas.cpp
  src/ui/widgets.cpp
  src/ui/app.cpp
  src/ui/calc_screen.cpp
  src/ui/graph_screen.cpp
  src/ui/matrix_screen.cpp
  src/ui/stat_screen.cpp
  src/ui/prog_screen.cpp
  src/ui/solve_screen.cpp
  src/ui/units_screen.cpp
  src/ui/help_screen.cpp
  src/ui/mode_screen.cpp
)
target_link_libraries(cactulator PRIVATE cactcore ${CURSES_LIBRARIES})
target_include_directories(cactulator PRIVATE ${CURSES_INCLUDE_DIRS})
if(CURSES_CFLAGS)
  target_compile_options(cactulator PRIVATE ${CURSES_CFLAGS})
endif()

if(MSVC)
  target_compile_options(cactcore PRIVATE /W4 /utf-8)
  target_compile_options(cactulator PRIVATE /W4 /utf-8)
else()
  target_compile_options(cactcore PRIVATE -Wall -Wextra)
  target_compile_options(cactulator PRIVATE -Wall -Wextra)
endif()

install(TARGETS cactulator RUNTIME DESTINATION bin)

# --- tests --------------------------------------------------------------------
enable_testing()
add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE cactcore)
if(MSVC)
  target_compile_options(test_core PRIVATE /utf-8)
endif()
add_test(NAME core COMMAND test_core WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
