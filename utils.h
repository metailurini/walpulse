#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>

int report_error(const char* message, int fatal);
uint16_t to_host16(uint16_t big_endian);
uint32_t to_host32(uint32_t big_endian);
uint64_t to_host64(uint64_t big_endian);
void print_hex_dump(const uint8_t* data, uint32_t size, uint32_t max_bytes);
int64_t parse_varint(const uint8_t* data, size_t* pos, size_t max_pos, int* bytes_read);
void compute_wal_checksum(uint8_t* data, size_t len, uint32_t* checksum1, uint32_t* checksum2);
void capture_hex_dump(const uint8_t* data, uint32_t size, uint32_t max_bytes, char* buffer, size_t buffer_size);

#endif