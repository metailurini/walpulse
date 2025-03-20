#include "utils.h"
#include <stdio.h>

// Reports an error message, optionally marking it as fatal
int report_error(const char *message, int fatal) {
    if (fatal) {
        perror(message);
        return -1;
    }
    printf("Error: %s\n", message);
    return 0;
}

// Converts big-endian 16-bit integer to host byte order
uint16_t to_host16(uint16_t big_endian) {
    return __builtin_bswap16(big_endian);
}

// Converts big-endian 32-bit integer to host byte order
uint32_t to_host32(uint32_t big_endian) {
    return __builtin_bswap32(big_endian);
}

// Converts big-endian 64-bit integer to host byte order
uint64_t to_host64(uint64_t big_endian) {
    return __builtin_bswap64(big_endian);
}

// Prints a hex dump of the data, limited to max_bytes
void print_hex_dump(const uint8_t *data, uint32_t size, uint32_t max_bytes) {
    uint32_t bytes_to_print = (size < max_bytes) ? size : max_bytes;
    printf("    First %u bytes (hex):\n    ", bytes_to_print);
    for (uint32_t i = 0; i < bytes_to_print; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n    ");
        }
    }
    printf("\n");
}

// Captures a hex dump into a buffer, limited to max_bytes
void capture_hex_dump(const uint8_t *data, uint32_t size, uint32_t max_bytes, char *buffer, size_t buffer_size) {
    uint32_t bytes_to_print = (size < max_bytes) ? size : max_bytes;
    size_t offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "    First %u bytes (hex):\n    ", bytes_to_print);
    for (uint32_t i = 0; i < bytes_to_print && offset < buffer_size; i++) {
        offset += snprintf(buffer + offset, buffer_size - offset, "%02x ", data[i]);
        if ((i + 1) % 16 == 0) {
            offset += snprintf(buffer + offset, buffer_size - offset, "\n    ");
        }
    }
}

// Parses a SQLite varint from data at position pos
int64_t parse_varint(const uint8_t *data, size_t *pos, size_t max_pos, int *bytes_read) {
    int64_t result = 0;
    *bytes_read = 0;

    if (*pos >= max_pos) {
        return -1;
    }

    for (int i = 0; i < 9 && *pos < max_pos; i++) {
        uint8_t byte = data[*pos];
        (*pos)++;
        (*bytes_read)++;
        result = (result << 7) | (byte & 0x7F);
        if (byte < 0x80) {
            return result;
        }
    }
    return -1;
}

// Computes SQLite WAL checksums for the given data
void compute_wal_checksum(uint8_t *data, size_t len, uint32_t *checksum1, uint32_t *checksum2) {
    uint32_t s1 = *checksum1;
    uint32_t s2 = *checksum2;
    for (size_t i = 0; i < len; i++) {
        s1 += data[i];
        s2 += s1;
    }
    *checksum1 = s1;
    *checksum2 = s2;
}