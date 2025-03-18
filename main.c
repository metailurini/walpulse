#include "wal_parser.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        if (report_error("Usage: <program> <database.db-wal>", 1) < 0) {
            return 1;
        }
    }
    int ret = print_wal_info(argv[1]);
    return ret == 0 ? 0 : 1;
}