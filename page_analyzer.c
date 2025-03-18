#include "page_analyzer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

void print_page_type(uint8_t* page_data, uint32_t page_number) {
    uint8_t page_type = page_data[0];
    printf("  Page Type: ");
    switch (page_type) {
        case 0x02: printf("B-tree Index Interior\n"); break;
        case 0x05: printf("B-tree Table Interior\n"); break;
        case 0x0A: printf("B-tree Index Leaf\n"); break;
        case 0x0D: printf("B-tree Table Leaf\n"); break;
        case 0x00:
            if (page_number == 1) {
                printf("Database Header\n");
            } else {
                printf("Freelist or Unused\n");
            }
            break;
        default: printf("Unknown (0x%02x)\n", page_type); break;
    }
}

void print_page_header(uint8_t* page_data, uint32_t page_number, uint32_t page_size) {
    uint8_t page_type = page_data[0];
    uint16_t freeblock_offset = to_host16(*(uint16_t*)(page_data + 1));
    uint16_t cell_count = to_host16(*(uint16_t*)(page_data + 3));
    uint16_t content_start = to_host16(*(uint16_t*)(page_data + 5));
    uint8_t fragmented_bytes = page_data[7];

    uint8_t* header_start = page_data;
    if (page_number == 1 && page_type == 0x00) {
        header_start = page_data + 100;
        freeblock_offset = to_host16(*(uint16_t*)(header_start + 1));
        cell_count = to_host16(*(uint16_t*)(header_start + 3));
        content_start = to_host16(*(uint16_t*)(header_start + 5));
        fragmented_bytes = header_start[7];
    }

    printf("  Page Header:\n");
    printf("    First Freeblock Offset: %u (0 if no freeblocks)\n", freeblock_offset);
    printf("    Number of Cells: %u\n", cell_count);
    printf("    Cell Content Start: %u (0 if uninitialized, defaults to %u)\n",
           content_start, content_start == 0 ? page_size : content_start);
    printf("    Fragmented Free Bytes: %u\n", fragmented_bytes);

    if (page_type == 0x02 || page_type == 0x05) {
        uint32_t rightmost_child = to_host32(*(uint32_t*)(header_start + 8));
        printf("    Rightmost Child Page: %u\n", rightmost_child);
    }

    if (page_type == 0x0D) {
        if (cell_count == 0) {
            printf("    No cells to display.\n");
            return;
        }
        printf("    Cells (%u):\n", cell_count);

        uint32_t pointer_array_size = 8 + cell_count * 2;
        if (pointer_array_size > page_size) {
            report_error("Cell pointer array exceeds page size", 0);
            return;
        }

        uint16_t* cell_pointers = malloc(cell_count * sizeof(uint16_t));
        if (!cell_pointers) {
            report_error("Failed to allocate memory for cell pointers", 0);
            return;
        }

        for (uint16_t i = 0; i < cell_count; i++) {
            size_t offset = 8 + (i * 2);
            cell_pointers[i] = to_host16(*(uint16_t*)(page_data + offset));
            if (cell_pointers[i] >= page_size) {
                report_error("Invalid cell pointer exceeds page size", 0);
                free(cell_pointers);
                return;
            }
        }

        for (uint16_t i = 0; i < cell_count; i++) {
            CellInfo cell = parse_cell(page_data, cell_pointers[i], page_size);
            if (cell.payload_size >= 0) {  // Only print valid cells
                print_cell_info(&cell, page_data, page_size);
            }
            free_cell_info(&cell);
        }
        free(cell_pointers);
    }
}

CellInfo parse_cell(uint8_t* page_data, uint32_t offset, uint32_t page_size) {
    CellInfo cell = { .offset = offset, .serial_types = NULL };
    size_t pos = offset;
    int bytes_read;

    if (pos >= page_size) {
        report_error("Cell offset exceeds page size", 0);
        cell.payload_size = -1;
        return cell;
    }

    cell.payload_size = parse_varint(page_data, &pos, page_size, &bytes_read);
    if (cell.payload_size < 0) return cell;

    cell.rowid = parse_varint(page_data, &pos, page_size, &bytes_read);
    if (cell.rowid < 0) {
        cell.payload_size = -1;
        return cell;
    }

    cell.header_size = parse_varint(page_data, &pos, page_size, &bytes_read);
    if (cell.header_size < 0 || pos + cell.header_size > page_size) {
        cell.payload_size = -1;
        return cell;
    }

    size_t header_start = pos;
    size_t header_end = header_start + cell.header_size;
    cell.column_count = 0;

    #define MAX_COLUMNS 1024
    cell.serial_types = malloc(MAX_COLUMNS * sizeof(int64_t));
    if (!cell.serial_types) {
        report_error("Failed to allocate memory for serial types", 0);
        cell.payload_size = -1;
        return cell;
    }

    size_t current_pos = header_start;
    while (current_pos < header_end && cell.column_count < MAX_COLUMNS) {
        int64_t serial_type = parse_varint(page_data, &current_pos, page_size, &bytes_read);
        if (serial_type < 0) {
            free(cell.serial_types);
            cell.serial_types = NULL;
            cell.payload_size = -1;
            return cell;
        }
        cell.serial_types[cell.column_count++] = serial_type;
    }

    if (current_pos != header_end) {
        report_error("Header size mismatch", 0);
        free(cell.serial_types);
        cell.serial_types = NULL;
        cell.payload_size = -1;
    }

    return cell;
}

void print_cell_info(CellInfo* cell, uint8_t* page_data, uint32_t page_size) {
    printf("      Cell at offset %u:\n", cell->offset);
    printf("        Payload Size: %lld bytes\n", cell->payload_size);
    printf("        RowID: %lld\n", cell->rowid);
    printf("        Number of Columns: %u\n", cell->column_count);

    size_t value_pos = cell->offset + (cell->header_size + 3); // Approximate varint sizes
    size_t payload_end = value_pos + cell->payload_size;
    if (payload_end > page_size) {
        printf("        Warning: Payload size exceeds page boundary\n");
        return;
    }

    printf("        Column Details:\n");
    for (uint32_t j = 0; j < cell->column_count; j++) {
        const char* type_name;
        uint32_t length;
        if (parse_serial_type(cell->serial_types[j], &type_name, &length) < 0) {
            printf("          Column %u: Error: Unknown serial type %lld\n", j + 1, cell->serial_types[j]);
            continue;
        }
        printf("          Column %u: Type: %s, Length: %u bytes, Value: ", j + 1, type_name, length);
        print_column_value(page_data, value_pos, page_size, type_name, length);
        printf("\n");
        value_pos += length;
    }

    if (value_pos < payload_end) {
        uint32_t remaining = payload_end - value_pos;
        printf("        Remaining Payload (%u bytes):\n        ", remaining);
        print_hex_dump(page_data + value_pos, remaining, 32);
    }
}

void free_cell_info(CellInfo* cell) {
    if (cell->serial_types) {
        free(cell->serial_types);
        cell->serial_types = NULL;
    }
}

int parse_serial_type(int64_t serial_type, const char** type_name, uint32_t* length) {
    if (serial_type == 0) { *type_name = "NULL"; *length = 0; return 0; }
    if (serial_type == 1) { *type_name = "INT8"; *length = 1; return 0; }
    if (serial_type == 2) { *type_name = "INT16"; *length = 2; return 0; }
    if (serial_type == 3) { *type_name = "INT24"; *length = 3; return 0; }
    if (serial_type == 4) { *type_name = "INT32"; *length = 4; return 0; }
    if (serial_type == 5) { *type_name = "INT48"; *length = 6; return 0; }
    if (serial_type == 6) { *type_name = "INT64"; *length = 8; return 0; }
    if (serial_type == 7) { *type_name = "FLOAT64"; *length = 8; return 0; }
    if (serial_type == 8) { *type_name = "ZERO"; *length = 0; return 0; }
    if (serial_type == 9) { *type_name = "ONE"; *length = 0; return 0; }
    if (serial_type >= 12 && serial_type % 2 == 0) {
        *type_name = "TEXT"; *length = (serial_type - 12) / 2; return 0;
    }
    if (serial_type >= 13 && serial_type % 2 == 1) {
        *type_name = "BLOB"; *length = (serial_type - 13) / 2; return 0;
    }
    return -1;
}

void print_column_value(const uint8_t* data, size_t pos, size_t max_pos, const char* type_name, uint32_t length) {
    if (pos + length > max_pos) { report_error("Value exceeds page size", 0); return; }
    if (strcmp(type_name, "NULL") == 0) printf("NULL");
    else if (strcmp(type_name, "INT8") == 0) printf("%d", (int8_t)data[pos]);
    else if (strcmp(type_name, "INT16") == 0) printf("%d", (int16_t)to_host16(*(uint16_t*)(data + pos)));
    else if (strcmp(type_name, "INT24") == 0) printf("%d", (int32_t)((data[pos] << 16) | (data[pos+1] << 8) | data[pos+2]));
    else if (strcmp(type_name, "INT32") == 0) printf("%d", (int32_t)to_host32(*(uint32_t*)(data + pos)));
    else if (strcmp(type_name, "INT64") == 0) printf("%lld", (int64_t)to_host64(*(uint64_t*)(data + pos)));
    else if (strcmp(type_name, "FLOAT64") == 0) printf("%f", *(double*)(data + pos));
    else if (strcmp(type_name, "TEXT") == 0) {
        printf("\"");
        for (uint32_t i = 0; i < length; i++) printf("%c", data[pos + i]);
        printf("\"");
    }
    else if (strcmp(type_name, "BLOB") == 0) printf("BLOB(%u bytes)", length);
    else printf("Unknown type");
}
