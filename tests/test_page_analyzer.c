#include "../page_analyzer.h"
#include "test_harness.h"
#include <string.h>

TEST(test_parse_serial_type) {
    const char* type_name;
    uint32_t length;

    int result = parse_serial_type(0, &type_name, &length);
    ASSERT(result == 0);
    ASSERT(strcmp(type_name, "NULL") == 0);
    ASSERT(length == 0);

    result = parse_serial_type(1, &type_name, &length);
    ASSERT(result == 0);
    ASSERT(strcmp(type_name, "INT8") == 0);
    ASSERT(length == 1);

    result = parse_serial_type(12, &type_name, &length);
    ASSERT(result == 0);
    ASSERT(strcmp(type_name, "TEXT") == 0);
    ASSERT(length == 0);

    result = parse_serial_type(13, &type_name, &length);
    ASSERT(result == 0);
    ASSERT(strcmp(type_name, "BLOB") == 0);
    ASSERT(length == 0);

    result = parse_serial_type(11, &type_name, &length);
    ASSERT(result == -1);
}

void register_page_analyzer_tests(void) {
    run_test("test_parse_serial_type", test_parse_serial_type);
}
