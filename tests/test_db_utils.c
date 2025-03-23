#include "../db_utils.h"
#include "test_harness.h"
#include <string.h>

TEST(test_get_table_name_from_page) {
    // This test requires a real SQLite database file, which isn't provided.
    // For now, test with a mock failure case assuming NULL on error.
    char* table_name = get_table_name_from_page("./tests/testdata/nonexistent.db", 1);
    ASSERT(table_name == NULL);
}

void register_db_utils_tests(void) {
    run_test("test_get_table_name_from_page", test_get_table_name_from_page);
}
