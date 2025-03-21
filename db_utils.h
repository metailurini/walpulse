#ifndef DB_UTILS_H
#define DB_UTILS_H

#include <stdint.h>

// Returns the table name for a given page number, or NULL if not found or on error
char *get_table_name_from_page(const char *db_filename, uint32_t page_number);

#endif
