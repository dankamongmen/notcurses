#ifndef NOTCURSES_TEST_MAIN
#define NOTCURSES_TEST_MAIN

#include <unistd.h>
#include "version.h"
#include <doctest/doctest.h>
#include <notcurses/notcurses.h>
#include <ncpp/NotCurses.hh>
#include <ncpp/_exceptions.hh>
#include "internal.h"

auto find_data(const char* datum) -> char*;
auto enforce_utf8() -> bool;

#endif
