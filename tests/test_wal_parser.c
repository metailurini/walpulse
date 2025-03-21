#include "../wal_parser.h"
#include "test_harness.h"
#include <string.h>

TEST(test_read_wal_header) {
    uint8_t buffer[] = {
        0x37, 0x7F, 0x06, 0x82, // magic: 0x377f0682
        0x00, 0x00, 0x00, 0x01, // format: 1
        0x00, 0x00, 0x10, 0x00, // page_size: 4096
        0x00, 0x00, 0x00, 0x02, // checkpoint: 2
        0x12, 0x34, 0x56, 0x78, // salt1: 0x12345678
        0x87, 0x65, 0x43, 0x21, // salt2: 0x87654321
        0xAA, 0xBB, 0xCC, 0xDD, // checksum1: 0xaabbccdd
        0xEE, 0xFF, 0x11, 0x22  // checksum2: 0xeeff1122
    };
    FILE* file = fmemopen(buffer, sizeof(buffer), "rb");
    ASSERT(file != NULL);
    WalHeader header = read_wal_header(file);
    fclose(file);
    ASSERT(header.magic == 0x377f0682);
    ASSERT(header.format == 1);
    ASSERT(header.page_size == 4096);
    ASSERT(header.checkpoint == 2);
    ASSERT(header.salt1 == 0x12345678);
    ASSERT(header.salt2 == 0x87654321);
    ASSERT(header.checksum1 == 0xaabbccdd);
    ASSERT(header.checksum2 == 0xeeff1122);
}

TEST(test_validate_wal_file_size) {
    long file_size = 32 + 24 + 4096; // Header + FrameHeader + 4096 page
    int frame_count = validate_wal_file_size(file_size, 4096);
    ASSERT(frame_count == 1);

    long invalid_size = 32 + 24 + 4090; // Misaligned
    frame_count = validate_wal_file_size(invalid_size, 4096);
    ASSERT(frame_count >= 0); // Error reported, but return value is still computed
}

void register_wal_parser_tests(void) {
    run_test("test_read_wal_header", test_read_wal_header);
    run_test("test_validate_wal_file_size", test_validate_wal_file_size);
}
