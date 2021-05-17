#ifndef NOTCURSES_TEST_MAIN
#define NOTCURSES_TEST_MAIN

#include <unistd.h>
#include "version.h"
#include "builddef.h"
#include <doctest/doctest.h>
#include <notcurses/notcurses.h>
#include <ncpp/NotCurses.hh>
#include <ncpp/_exceptions.hh>
#include "internal.h"

auto is_test_tty() -> bool;
auto find_data(const char* datum) -> char*;
auto testing_notcurses() -> struct notcurses*;
auto ncreel_validate(const ncreel* n) -> bool;

#endif
