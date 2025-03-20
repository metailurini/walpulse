#include "wal_parser.h"
#include "utils.h"
#include "page_analyzer.h"
#include <stdio.h>
#include <stdlib.h>

// Reads and validates the WAL file header
WalHeader read_wal_header(FILE *file) {
    WalHeader header = {0};
    if (fread(&header, sizeof(WalHeader), 1, file) != 1) {
        report_error("Could not read WAL header", 1);
        return header;
    }

    // Convert header fields from big-endian to host byte order
    header.magic = to_host32(header.magic);
    header.format = to_host32(header.format);
    header.page_size = to_host32(header.page_size);
    header.checkpoint = to_host32(header.checkpoint);
    header.salt1 = to_host32(header.salt1);
    header.salt2 = to_host32(header.salt2);
    header.checksum1 = to_host32(header.checksum1);
    header.checksum2 = to_host32(header.checksum2);

    // Validate magic number
    if (header.magic != 0x377f0682 && header.magic != 0x377f0683) {
        report_error("Invalid WAL file: incorrect magic number", 1);
    }
    return header;
}

// Validates WAL file size and returns the number of frames
int validate_wal_file_size(long file_size, uint32_t page_size) {
    long remaining_size = file_size - sizeof(WalHeader);
    int frame_size = sizeof(FrameHeader) + page_size;
    if (remaining_size % frame_size != 0) {
        report_error("Corrupt WAL file - Unexpected file size", 1);
    }
    return remaining_size / frame_size;
}

// Processes and prints information about WAL frames
void process_wal_frames(FILE *file, WalHeader *header) {
    FrameHeader frame;
    uint32_t frame_count = 0;
    uint8_t *page_data = malloc(header->page_size);
    if (!page_data) {
        report_error("Could not allocate memory for page data", 1);
        return;
    }

    printf("Frame Information:\n");
    while (fread(&frame, sizeof(FrameHeader), 1, file) == 1) {
        // Convert frame fields from big-endian to host byte order
        frame.page_number = to_host32(frame.page_number);
        frame.commit_size = to_host32(frame.commit_size);
        frame.salt1 = to_host32(frame.salt1);
        frame.salt2 = to_host32(frame.salt2);
        frame.checksum1 = to_host32(frame.checksum1);
        frame.checksum2 = to_host32(frame.checksum2);

        // Read page data for the frame
        if (fread(page_data, header->page_size, 1, file) != 1) {
            printf("Warning: Incomplete frame data for frame %u\n", frame_count + 1);
            break;
        }

        frame_count++;
        printf("Frame %u:\n", frame_count);
        printf("  Page Number: %u\n", frame.page_number);
        printf("  Commit Size: %u pages (0 if not a commit frame)\n", frame.commit_size);
        printf("  Salt-1: 0x%08x\n", frame.salt1);
        printf("  Salt-2: 0x%08x\n", frame.salt2);
        printf("  Checksum-1: 0x%08x\n", frame.checksum1);
        printf("  Checksum-2: 0x%08x\n", frame.checksum2);

        print_page_type(page_data, frame.page_number);
        verify_frame_checksum(&frame, page_data, header->page_size, frame.checksum1, frame.checksum2);
        print_page_header(page_data, frame.page_number, header->page_size);
        print_hex_dump(page_data, header->page_size, 32);
        printf("\n");
    }

    // Print summary of frames processed
    if (frame_count == 0) {
        printf("No frames found in the WAL file.\n");
    } else {
        printf("Total frames: %u\n", frame_count);
    }
    free(page_data);
}

// Prints detailed information about the WAL file
int print_wal_info(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return report_error("Failed to open WAL file", 1);
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Validate file size
    if (file_size < sizeof(WalHeader)) {
        fclose(file);
        return report_error("File too small to be a WAL file", 1);
    }

    // Read and process header
    WalHeader header = read_wal_header(file);
    validate_wal_file_size(file_size, header.page_size);

    // Print header information
    printf("WAL File Information for %s:\n", filename);
    printf("Magic Number: 0x%08x\n", header.magic);
    printf("File Format: %u\n", header.format);
    printf("Page Size: %u bytes\n", header.page_size);
    printf("Checkpoint Sequence: %u\n", header.checkpoint);
    printf("Salt-1: 0x%08x\n", header.salt1);
    printf("Salt-2: 0x%08x\n", header.salt2);
    printf("Checksum-1: 0x%08x\n", header.checksum1);
    printf("Checksum-2: 0x%08x\n", header.checksum2);
    printf("\n");

    // Process frames
    process_wal_frames(file, &header);
    fclose(file);
    return 0;
}

// Verifies the checksum of a frame against computed values
void verify_frame_checksum(FrameHeader *frame, uint8_t *page_data, uint32_t page_size,
                          uint32_t initial_checksum1, uint32_t initial_checksum2) {
    uint32_t computed_checksum1 = initial_checksum1;
    uint32_t computed_checksum2 = initial_checksum2;

    compute_wal_checksum((uint8_t *)frame, sizeof(FrameHeader), &computed_checksum1, &computed_checksum2);
    compute_wal_checksum(page_data, page_size, &computed_checksum1, &computed_checksum2);

    if (computed_checksum1 != frame->checksum1 || computed_checksum2 != frame->checksum2) {
        printf("Checksum mismatch for frame! Expected: 0x%08x 0x%08x, Computed: 0x%08x 0x%08x\n",
               frame->checksum1, frame->checksum2, computed_checksum1, computed_checksum2);
    } else {
        printf("Checksum verified successfully.\n");
    }
}
