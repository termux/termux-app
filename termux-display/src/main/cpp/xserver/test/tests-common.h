#ifndef TESTS_COMMON_H
#define TESTS_COMMON_H

#include "tests.h"

#define ARRAY_SIZE(a)  (sizeof((a)) / sizeof((a)[0]))

#define run_test(func) run_test_in_child(func, #func)

void run_test_in_child(int (*func)(void), const char *funcname);

#endif /* TESTS_COMMON_H */
