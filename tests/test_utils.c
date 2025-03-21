#include "../utils.h"
#include "test_harness.h"

TEST(test_to_host32) {
    uint32_t big_endian = 0x12345678;
    uint32_t expected = 0x78563412; // Assuming little-endian host
    ASSERT(to_host32(big_endian) == expected);
}

TEST(test_parse_varint) {
    uint8_t data1[] = {0x7F}; // Single-byte varint: 127
    size_t pos1 = 0;
    int bytes_read1;
    int64_t result1 = parse_varint(data1, &pos1, sizeof(data1), &bytes_read1);
    ASSERT(result1 == 127);
    ASSERT(pos1 == 1);
    ASSERT(bytes_read1 == 1);

    uint8_t data2[] = {0x81, 0x80, 0x01}; // Multi-byte varint: 16385
    size_t pos2 = 0;
    int bytes_read2;
    int64_t result2 = parse_varint(data2, &pos2, sizeof(data2), &bytes_read2);
    ASSERT(result2 == 16385);
    ASSERT(pos2 == 3);
    ASSERT(bytes_read2 == 3);

    uint8_t data3[] = {0xFF}; // Incomplete multi-byte, should fail
    size_t pos3 = 0;
    int bytes_read3;
    int64_t result3 = parse_varint(data3, &pos3, sizeof(data3), &bytes_read3);
    ASSERT(result3 == -1);
}

TEST(test_compute_wal_checksum) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    uint32_t checksum1 = 0;
    uint32_t checksum2 = 0;
    compute_wal_checksum(data, sizeof(data), &checksum1, &checksum2);
    ASSERT(checksum1 == 0x06); // 1 + 2 + 3
    ASSERT(checksum2 == 0x0A); // (0+1) + (1+2) + (1+2+3)
}

TEST(test_report_error) {
    int result = report_error("Test error", 0);
    ASSERT(result == 0); // Non-fatal should return 0
    result = report_error("Fatal test error", 1);
    ASSERT(result == -1); // Fatal should return 1
}

TEST(test_to_host16) {
    uint16_t big_endian = 0x1234;
    uint16_t expected = 0x3412; // Assuming little-endian host
    ASSERT(to_host16(big_endian) == expected);
}

TEST(test_to_host64) {
    uint64_t big_endian = 0x123456789ABCDEF0;
    uint64_t expected = 0xF0DEBC9A78563412; // Assuming little-endian host
    ASSERT(to_host64(big_endian) == expected);
}

TEST(test_print_hex_dump) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    print_hex_dump(data, 4, 4);
    // No direct assertions possible due to output-only function; test for no crash
}

void register_utils_tests(void) {
    run_test("test_to_host32", test_to_host32);
    run_test("test_parse_varint", test_parse_varint);
    run_test("test_compute_wal_checksum", test_compute_wal_checksum);
    run_test("test_report_error", test_report_error);
    run_test("test_to_host16", test_to_host16);
    run_test("test_to_host64", test_to_host64);
    run_test("test_print_hex_dump", test_print_hex_dump);
}
