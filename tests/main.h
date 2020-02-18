#ifndef NOTCURSES_TEST_MAIN
#define NOTCURSES_TEST_MAIN

#include <unistd.h>
#include <notcurses.h>
#include <doctest/doctest.h>

char* find_data(const char* datum);

// some tests can only run in a utf8 environment
bool enforce_utf8();

#endif
