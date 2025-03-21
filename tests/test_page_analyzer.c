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

TEST(test_parse_cell) {
    uint8_t page_data[] = {
        0x03, // Payload size: 3 bytes
        0x01, // RowID: 1
        0x02, // Header size: 2 bytes
        0x01, // Serial type: INT8
        0x02  // Serial type: INT16
    };
    CellInfo cell = parse_cell(page_data, 0, sizeof(page_data));
    ASSERT(cell.payload_size == 3);
    ASSERT(cell.rowid == 1);
    ASSERT(cell.header_size == 2);
    printf("Header size: %lld\n", cell.header_size);
    ASSERT(cell.column_count == 2);
    ASSERT(cell.serial_types[0] == 1);
    ASSERT(cell.serial_types[1] == 2);
    free_cell_info(&cell);
}

TEST(test_print_page_header) {
    uint8_t page_data[] = {
        0x0D, // Leaf table b-tree page
        0x00, 0x00, // Freeblock offset
        0x00, 0x02, // Cell count: 2
        0x03, 0xFF, // Cell content area start
        0x00  // Fragmented bytes
    };
    print_page_header(page_data, 1, 1024, "test.db");
    // No direct assertions possible due to output-only function; test for no crash
}

void register_page_analyzer_tests(void) {
    run_test("test_parse_serial_type", test_parse_serial_type);
    run_test("test_parse_cell", test_parse_cell);
    run_test("test_print_page_header", test_print_page_header);
}