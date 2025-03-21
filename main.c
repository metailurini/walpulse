#include "wal_parser.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

// Main entry point for the database and WAL file parser
int main(int argc, char *argv[]) {
    // Check for correct number of arguments
    if (argc != 2) {
        report_error("Usage: <program> <database.db>", 1);
        return 1;
    }

    const char *db_filename = argv[1];
    // Compute WAL filename by appending "-wal"
    size_t db_len = strlen(db_filename);
    char *wal_filename = malloc(db_len + 5); // "-wal" + null terminator
    if (!wal_filename) {
        report_error("Failed to allocate memory for WAL filename", 1);
        return 1;
    }
    strcpy(wal_filename, db_filename);
    strcpy(wal_filename + db_len, "-wal");

    // Process the WAL file and return appropriate status
    int status = print_wal_info(wal_filename);
    free(wal_filename);
    return status == 0 ? 0 : 1;
}
