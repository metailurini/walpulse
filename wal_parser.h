#ifndef WAL_PARSER_H
#define WAL_PARSER_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t magic;
    uint32_t format;
    uint32_t page_size;
    uint32_t checkpoint;
    uint32_t salt1;
    uint32_t salt2;
    uint32_t checksum1;
    uint32_t checksum2;
} WalHeader;

typedef struct {
    uint32_t page_number;
    uint32_t commit_size;
    uint32_t salt1;
    uint32_t salt2;
    uint32_t checksum1;
    uint32_t checksum2;
} FrameHeader;

WalHeader read_wal_header(FILE* file);
int validate_wal_file_size(long file_size, uint32_t page_size);
void process_wal_frames(FILE* file, WalHeader* header);
int print_wal_info(const char* filename);
void verify_frame_checksum(FrameHeader* frame, uint8_t* page_data, uint32_t page_size,
                          uint32_t initial_checksum1, uint32_t initial_checksum2);

#endif