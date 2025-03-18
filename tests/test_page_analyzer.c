#include "../page_analyzer.h"
#include "test_harness.h"

TEST(test_parse_serial_type) {
    const char* type_name;
    uint32_t length;
    int result = parse_serial_type(0, &type_name, &length);
    ASSERT(result == 0 && length == 0);
}

void register_page_analyzer_tests(void) {
    run_test("test_parse_serial_type", test_parse_serial_type);
}