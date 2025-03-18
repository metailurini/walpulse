#ifndef PAGE_ANALYZER_H
#define PAGE_ANALYZER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t offset;
    int64_t payload_size;
    int64_t rowid;
    int64_t header_size;
    uint32_t column_count;
    int64_t* serial_types;
} CellInfo;

void print_page_type(uint8_t* page_data, uint32_t page_number);
void print_page_header(uint8_t* page_data, uint32_t page_number, uint32_t page_size);
CellInfo parse_cell(uint8_t* page_data, uint32_t offset, uint32_t page_size);
void print_cell_info(CellInfo* cell, uint8_t* page_data, uint32_t page_size);
void free_cell_info(CellInfo* cell);
int parse_serial_type(int64_t serial_type, const char** type_name, uint32_t* length);
void print_column_value(const uint8_t* data, size_t pos, size_t max_pos, const char* type_name, uint32_t length);

#endif
