#include "test_harness.h"

void register_wal_parser_tests(void);
void register_utils_tests(void);
void register_page_analyzer_tests(void);

void run_all_tests(void) {
    register_wal_parser_tests();
    register_utils_tests();
    register_page_analyzer_tests();
}

int main(void) {
    run_all_tests();
    return 0;
}