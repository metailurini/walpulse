#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#define TEST(name) void name(void)
#define ASSERT(cond) do { if (!(cond)) { printf("Assertion failed: %s, %s:%d\n", #cond, __FILE__, __LINE__); test_failures++; }} while(0)

typedef void (*test_func)(void);

void run_test(const char* name, test_func func);
void run_all_tests(void);
extern int test_failures;

#endif