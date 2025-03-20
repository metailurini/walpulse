#include "wal_parser.h"
#include "utils.h"

// Main entry point for the WAL file parser
int main(int argc, char *argv[]) {
    // Check for correct number of arguments
    if (argc != 2) {
        report_error("Usage: <program> <database.db-wal>", 1);
        return 1;
    }

    // Process the WAL file and return appropriate status
    return print_wal_info(argv[1]) == 0 ? 0 : 1;
}