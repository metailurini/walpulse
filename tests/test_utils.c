#include "../utils.h"
#include "test_harness.h"

TEST(test_to_host32) {
    ASSERT(to_host32(0x12345678) != 0x12345678); // Basic endianness check
}

void register_utils_tests(void) {
    run_test("test_to_host32", test_to_host32);
}
