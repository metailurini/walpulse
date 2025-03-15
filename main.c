#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// WAL Header Structure (32 bytes)
typedef struct {
    uint32_t magic;        // Magic number (0x377f0682 or 0x377f0683)
    uint32_t format;       // File format version (3007000)
    uint32_t page_size;    // Database page size
    uint32_t checkpoint;   // Checkpoint sequence number
    uint32_t salt1;        // Random salt 1
    uint32_t salt2;        // Random salt 2
    uint32_t checksum1;    // Checksum part 1
    uint32_t checksum2;    // Checksum part 2
} WalHeader;

// Frame Header Structure (24 bytes)
typedef struct {
    uint32_t page_number;  // Page number in the database
    uint32_t commit_size;  // Size of database after commit (0 if not a commit frame)
    uint32_t salt1;        // Salt-1 from WAL header at frame creation
    uint32_t salt2;        // Salt-2 from WAL header at frame creation
    uint32_t checksum1;    // Checksum part 1
    uint32_t checksum2;    // Checksum part 2
} FrameHeader;

// Function to print a hex dump of the first N bytes of data
void print_hex_dump(const uint8_t* data, uint32_t size, uint32_t max_bytes) {
    uint32_t bytes_to_print = (size < max_bytes) ? size : max_bytes;
    printf("    First %u bytes (hex):\n    ", bytes_to_print);
    for (uint32_t i = 0; i < bytes_to_print; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n    ");
    }
    printf("\n");
}

// Function to parse a varint (variable-length integer)
int64_t parse_varint(const uint8_t *data, size_t *pos, size_t max_pos, int *bytes_read) {
    int64_t result = 0;
    *bytes_read = 0;
    for (int i = 0; i < 9 && *pos < max_pos; i++) {
        uint8_t byte = data[*pos];
        (*pos)++;
        (*bytes_read)++;
        result = (result << 7) | (byte & 0x7F);
        if (byte < 0x80) {
            break;
        }
    }
    return result;
}

// Function to print page type based on the first byte of page data
void print_page_type(uint8_t* page_data, uint32_t page_number) {
    uint8_t page_type = page_data[0];
    printf("  Page Type: ");
    switch (page_type) {
        case 0x02:
            printf("B-tree Index Interior\n");
            break;
        case 0x05:
            printf("B-tree Table Interior\n");
            break;
        case 0x0A:
            printf("B-tree Index Leaf\n");
            break;
        case 0x0D:
            printf("B-tree Table Leaf\n");
            break;
        case 0x00:
            if (page_number == 1) {
                printf("Database Header\n");
            } else {
                printf("Freelist or Unused\n");
            }
            break;
        default:
            printf("Unknown (0x%02x)\n", page_type);
            break;
    }
}

// Function to print B-tree table leaf cell information
void print_table_leaf_cells(uint8_t* page_data, uint32_t page_size, uint16_t cell_count) {
    if (cell_count == 0) {
        printf("    No cells to display.\n");
        return;
    }

    printf("    Cells (%u):\n", cell_count);

    // Check if cell pointer array fits within page_size
    if (8 + cell_count * 2 > page_size) {
        printf("    Error: Cell pointer array exceeds page size\n");
        return;
    }

    uint16_t *cell_pointers = malloc(cell_count * sizeof(uint16_t));
    if (!cell_pointers) {
        printf("    Error: Failed to allocate memory for cell pointers\n");
        return;
    }

    // Read cell pointer array (starts at offset 8, big-endian to native)
    for (uint16_t i = 0; i < cell_count; i++) {
        cell_pointers[i] = __builtin_bswap16(*(uint16_t*)(page_data + 8 + (i * 2)));
    }

    // Parse and print each cell
    for (uint16_t i = 0; i < cell_count; i++) {
        size_t pos = cell_pointers[i];
        int bytes_read;

        // Parse payload size and RowID varints
        int64_t payload_size = parse_varint(page_data, &pos, page_size, &bytes_read);
        pos += bytes_read;  // Advance pos after parsing payload_size
        int64_t rowid = parse_varint(page_data, &pos, page_size, &bytes_read);
        pos += bytes_read;  // Advance pos after parsing rowid

        // Check for invalid payload size
        if (payload_size < 0) {
            printf("      Cell %u: Error: Negative payload size (%lld)\n", i + 1, payload_size);
            continue;
        }

        printf("      Cell %u:\n", i + 1);
        printf("        Offset: %u\n", cell_pointers[i]);
        printf("        Payload Size: %lld bytes\n", payload_size);
        printf("        RowID: %lld\n", rowid);

        // Parse header size (varint after RowID) to find payload start
        int64_t header_size = parse_varint(page_data, &pos, page_size, &bytes_read);
        pos += bytes_read;  // Advance pos to start of actual payload data
        size_t payload_start = pos;

        // Print first 32 bytes of payload as hex (if available)
        uint32_t payload_to_print = (payload_size < 32) ? (uint32_t)payload_size : 32;
        if (payload_start + payload_size <= page_size) {
            printf("        Payload Data (first %u bytes):\n        ", payload_to_print);
            for (uint32_t j = 0; j < payload_to_print; j++) {
                printf("%02x ", page_data[payload_start + j]);
                if ((j + 1) % 16 == 0) printf("\n        ");
            }
            printf("\n");
        } else {
            printf("        Warning: Payload exceeds page size\n");
        }
    }

    free(cell_pointers);
}

// Function to parse and print page header
void print_page_header(uint8_t* page_data, uint32_t page_number, uint32_t page_size) {
    uint8_t page_type = page_data[0];
    uint16_t freeblock_offset = __builtin_bswap16(*(uint16_t*)(page_data + 1));
    uint16_t cell_count = __builtin_bswap16(*(uint16_t*)(page_data + 3));
    uint16_t content_start = __builtin_bswap16(*(uint16_t*)(page_data + 5));
    uint8_t fragmented_bytes = page_data[7];

    // Adjust for page 1 (database header has a 100-byte prefix)
    uint8_t* header_start = page_data;
    if (page_number == 1 && page_type == 0x00) {
        header_start = page_data + 100; // B-tree header starts after 100-byte database header
        freeblock_offset = __builtin_bswap16(*(uint16_t*)(header_start + 1));
        cell_count = __builtin_bswap16(*(uint16_t*)(header_start + 3));
        content_start = __builtin_bswap16(*(uint16_t*)(header_start + 5));
        fragmented_bytes = header_start[7];
    }

    printf("  Page Header:\n");
    printf("    First Freeblock Offset: %u (0 if no freeblocks)\n", freeblock_offset);
    printf("    Number of Cells: %u\n", cell_count);
    printf("    Cell Content Start: %u (0 if uninitialized, defaults to %u)\n", 
           content_start, content_start == 0 ? page_size : content_start);
    printf("    Fragmented Free Bytes: %u\n", fragmented_bytes);

    // For interior pages, print the rightmost child pointer
    if (page_type == 0x02 || page_type == 0x05) {
        uint32_t rightmost_child = __builtin_bswap32(*(uint32_t*)(header_start + 8));
        printf("    Rightmost Child Page: %u\n", rightmost_child);
    }

    // If B-tree table leaf page, print cell details
    if (page_type == 0x0D) {
        print_table_leaf_cells(page_data, page_size, cell_count);
    }
}

void print_wal_info(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open WAL file");
        return;
    }

    // Check file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    if (file_size < sizeof(WalHeader)) {
        printf("Error: File too small to be a WAL file (%ld bytes)\n", file_size);
        fclose(file);
        return;
    }

    // Read WAL Header
    WalHeader header;
    if (fread(&header, sizeof(WalHeader), 1, file) != 1) {
        printf("Error: Could not read WAL header\n");
        fclose(file);
        return;
    }

    // Convert header from big-endian to host byte order
    header.magic = __builtin_bswap32(header.magic);
    header.format = __builtin_bswap32(header.format);
    header.page_size = __builtin_bswap32(header.page_size);
    header.checkpoint = __builtin_bswap32(header.checkpoint);
    header.salt1 = __builtin_bswap32(header.salt1);
    header.salt2 = __builtin_bswap32(header.salt2);
    header.checksum1 = __builtin_bswap32(header.checksum1);
    header.checksum2 = __builtin_bswap32(header.checksum2);

    // Verify magic number
    if (header.magic != 0x377f0682 && header.magic != 0x377f0683) {
        printf("Invalid WAL file: incorrect magic number (0x%08x)\n", header.magic);
        fclose(file);
        return;
    }

    // Validate frame size consistency
    long remaining_size = file_size - sizeof(WalHeader);
    int frame_size = sizeof(FrameHeader) + header.page_size;
    
    if (remaining_size % frame_size != 0) {
        printf("Error: Corrupt WAL file - Unexpected file size (%ld bytes)\n", file_size);
        fclose(file);
        return;
    }

    // Print WAL Header
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

    // Read and process frames
    FrameHeader frame;
    uint32_t frame_count = 0;
    uint8_t* page_data = malloc(header.page_size); // Buffer for page data

    if (!page_data) {
        printf("Error: Could not allocate memory for page data\n");
        fclose(file);
        return;
    }

    printf("Frame Information:\n");
    while (fread(&frame, sizeof(FrameHeader), 1, file) == 1) {
        // Convert frame header from big-endian to host byte order
        frame.page_number = __builtin_bswap32(frame.page_number);
        frame.commit_size = __builtin_bswap32(frame.commit_size);
        frame.salt1 = __builtin_bswap32(frame.salt1);
        frame.salt2 = __builtin_bswap32(frame.salt2);
        frame.checksum1 = __builtin_bswap32(frame.checksum1);
        frame.checksum2 = __builtin_bswap32(frame.checksum2);

        // Read page data
        if (fread(page_data, header.page_size, 1, file) != 1) {
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
        
        // Print page type
        print_page_type(page_data, frame.page_number);
        
        // Print page header and cell data (if applicable)
        print_page_header(page_data, frame.page_number, header.page_size);
        
        // Print first 32 bytes of page data as a hex dump
        print_hex_dump(page_data, header.page_size, 32);
        printf("\n");
    }

    if (frame_count == 0) {
        printf("No frames found in the WAL file.\n");
    } else {
        printf("Total frames: %u\n", frame_count);
    }

    free(page_data);
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <database.db-wal>\n", argv[0]);
        return 1;
    }

    print_wal_info(argv[1]);
    return 0;
}
