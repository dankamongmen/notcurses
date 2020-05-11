#ifndef NOTCURSES_TEST_MAIN
#define NOTCURSES_TEST_MAIN

#include <unistd.h>
#include <doctest/doctest.h>
#include "version.h"
#include <notcurses/notcurses.h>

auto find_data(const char* datum) -> char*;
auto enforce_utf8() -> bool;

#endif
