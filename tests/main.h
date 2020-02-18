#ifndef NOTCURSES_TEST_MAIN
#define NOTCURSES_TEST_MAIN

#include <unistd.h>
#include <doctest/doctest.h>
#include "version.h"
#include <notcurses/notcurses.h>

char* find_data(const char* datum);

// some tests can only run in a utf8 environment
bool enforce_utf8();

#endif
