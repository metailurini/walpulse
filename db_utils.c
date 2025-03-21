#include "utils.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_table_name_from_page(const char *db_filename, uint32_t page_number) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *table_name = NULL;
    int rc;

    // Open the database
    rc = sqlite3_open(db_filename, &db);
    if (rc != SQLITE_OK) {
        report_error("Cannot open database", 0);
        sqlite3_close(db);
        return NULL;
    }

    // Prepare the SQL query
    const char *sql = "SELECT name FROM dbstat WHERE pageno = ?";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        report_error("Failed to prepare SQL statement", 0);
        sqlite3_close(db);
        return NULL;
    }

    // Bind the page number parameter
    sqlite3_bind_int(stmt, 1, page_number);

    // Execute the query and fetch the result
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(stmt, 0);
        if (name) {
            table_name = strdup(name); // Allocate and copy the table name
            if (!table_name) {
                report_error("Failed to allocate memory for table name", 0);
            }
        }
    }

    // Clean up
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return table_name;
}
