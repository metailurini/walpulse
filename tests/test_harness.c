#include "test_harness.h"

int test_failures = 0;

void run_test(const char* name, test_func func) {
    printf("\033[33mRunning %s...\033[0m\n", name); // Yellow
    func();
    if (test_failures == 0) {
        printf("\t\033[32m%s: PASSED\033[0m\n", name); // Green
    } else {
        printf("\t\033[31m%s: FAILED (%d failures)\033[0m\n", name, test_failures); // Red
    }
    test_failures = 0; // Reset for next test
}
