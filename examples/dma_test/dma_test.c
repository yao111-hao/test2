//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

#include "dma_utils.h"
#include <string.h> // For memcmp and memset

#define DEVICE_NAME_DEFAULT "/dev/qdma01000-MM-0"
#define SIZE_DEFAULT (32)
#define COUNT_DEFAULT (1)

extern int verbose;

// The options array for command-line argument parsing
static struct option const long_opts[] = {
    {"device", required_argument, NULL, 'd'},
    {"address", required_argument, NULL, 'a'},
    {"size", required_argument, NULL, 's'},
    {"offset", required_argument, NULL, 'o'},
    {"count", required_argument, NULL, 'c'},
    {"data infile", required_argument, NULL, 'f'},
    {"data outfile", required_argument, NULL, 'w'},
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {"read", no_argument, NULL, 'r'},
    {0, 0, 0, 0}
};

// Function prototypes for clarity
static void usage(const char *name);
static void dump_buffers(const char* golden_buffer, const char* received_buffer, uint64_t size);
static int test_dma_with_verification(char *devname, uint64_t addr, uint64_t size,
                                      uint64_t offset, uint64_t count);


/**
 * @brief RESTORED: This function prints the command-line usage instructions.
 */
static void usage(const char *name)
{
    int i = 0;
    fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);
    fprintf(stdout, "  -%c (--%s) device name (e.g. /dev/reconic-mm)\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) the start address on the AXI bus\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout,
            "  -%c (--%s) size of a single transfer in bytes, default %d,\n",
            long_opts[i].val, long_opts[i].name, SIZE_DEFAULT);
    i++;
    fprintf(stdout, "  -%c (--%s) page offset of transfer (currently unused in verification)\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) number of transfers, default %d\n",
            long_opts[i].val, long_opts[i].name, COUNT_DEFAULT);
    i++;
    fprintf(stdout, "  -%c (--%s) filename to read from (ignored in verification)\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) filename to write to (ignored in verification)\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) print usage help and exit\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) verbose output\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) read flag (ignored, test performs write-then-read)\n",
            long_opts[i].val, long_opts[i].name);
}

int main(int argc, char *argv[])
{
    int cmd_opt;
    char *device = DEVICE_NAME_DEFAULT;
    uint64_t address = 0;
    uint64_t size = SIZE_DEFAULT;
    uint64_t offset = 0;
    uint64_t count = COUNT_DEFAULT;
    
    while ((cmd_opt =
            getopt_long(argc, argv, "vhc:f:d:a:s:o:w:rq:i:", long_opts,
                        NULL)) != -1) {
        switch (cmd_opt) {
        case 0:
            /* long option */
            break;
        case 'd':
            device = strdup(optarg);
            break;
        case 'a':
            address = getopt_integer(optarg);
            break;
        case 's':
            size = getopt_integer(optarg);
            break;
        case 'o':
            offset = getopt_integer(optarg) & 4095;
            break;
        case 'c':
            count = getopt_integer(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        // Ignore flags that are no longer used by the verification test
        case 'r':
        case 'f':
        case 'w':
            break;
        default:
            usage(argv[0]);
            exit(0);
            break;
        }
    }

    if (verbose)
        fprintf(stdout,
        "Starting DMA test with verification.\nDevice: %s, Address: 0x%lx, Size: 0x%lx, Count: %lu\n",
        device, address, size, count);
    
    return test_dma_with_verification(device, address, size, offset, count);
}

/**
 * @brief NEW: Dumps the content of two buffers side-by-side for comparison.
 */
static void dump_buffers(const char* golden_buffer, const char* received_buffer, uint64_t size) {
    const uint64_t MAX_DUMP_SIZE = 256; // Limit dump to first 256 bytes
    uint64_t dump_size = (size > MAX_DUMP_SIZE) ? MAX_DUMP_SIZE : size;

    fprintf(stderr, "--------------------------------------------------\n");
    fprintf(stderr, "           Data Buffer Comparison Dump\n");
    fprintf(stderr, "--------------------------------------------------\n");
    fprintf(stderr, "Offset(h) | Expected (Golden) | Received (Actual) | Status\n");
    fprintf(stderr, "--------------------------------------------------\n");

    for (uint64_t i = 0; i < dump_size; i++) {
        unsigned char golden_char = (unsigned char)golden_buffer[i];
        unsigned char received_char = (unsigned char)received_buffer[i];
        const char* status = (golden_char == received_char) ? "" : "<<<<< MISMATCH";

        fprintf(stderr, "0x%08lx |       0x%02x        |        0x%02x       | %s\n",
                i, golden_char, received_char, status);
    }

    if (size > dump_size) {
        fprintf(stderr, "... (dump truncated to first %lu bytes) ...\n", dump_size);
    }
    fprintf(stderr, "--------------------------------------------------\n");
}

/**
 * @brief MODIFIED: Core function to perform DMA write-then-read with verification.
 */
static int test_dma_with_verification(char *devname, uint64_t addr, uint64_t size,
                                      uint64_t offset, uint64_t count)
{
    ssize_t rc;
    char *write_buffer = NULL;
    char *read_buffer = NULL;
    int fpga_fd;
    uint64_t i;

    fpga_fd = open(devname, O_RDWR);
    if (fpga_fd < 0) {
        fprintf(stderr, "Error: Unable to open device %s.\n", devname);
        perror("open device");
        return -EINVAL;
    }

    rc = posix_memalign((void **)&write_buffer, 4096, size);
    if (rc != 0 || !write_buffer) {
        fprintf(stderr, "Error: Could not allocate write buffer (OOM).\n");
        rc = -ENOMEM;
        goto out;
    }

    rc = posix_memalign((void **)&read_buffer, 4096, size);
    if (rc != 0 || !read_buffer) {
        fprintf(stderr, "Error: Could not allocate read buffer (OOM).\n");
        rc = -ENOMEM;
        goto out;
    }

    printf("Host buffers allocated. Write buffer at %p, Read buffer at %p.\n", write_buffer, read_buffer);

    for (i = 0; i < count; i++) {
        printf("\n--- Verification Cycle %lu of %lu ---\n", i + 1, count);

        // 1. Prepare golden data
        printf("Step 1: Preparing golden data pattern...\n");
        for (uint64_t j = 0; j < size; j++) {
            write_buffer[j] = (char)((i + j) & 0xFF);
        }

        // 2. Write from host to FPGA (H2C)
        printf("Step 2: Writing %lu bytes from host to FPGA at address 0x%lx...\n", size, addr);
        rc = write_from_buffer(devname, fpga_fd, write_buffer, size, addr);
        if (rc < 0) {
            fprintf(stderr, "Error: write_from_buffer failed on cycle %lu.\n", i + 1);
            goto out;
        }
        printf("         Write operation completed.\n");

        // 3. Read from FPGA back to host (C2H)
        memset(read_buffer, 0, size);
        printf("Step 3: Reading %lu bytes from FPGA at address 0x%lx back to host...\n", size, addr);
        rc = read_to_buffer(devname, fpga_fd, read_buffer, size, addr);
        if (rc < 0) {
            fprintf(stderr, "Error: read_to_buffer failed on cycle %lu.\n", i + 1);
            goto out;
        }
        printf("         Read operation completed.\n");

        // 4. Verify data integrity
        printf("Step 4: Verifying data integrity...\n");
        rc = memcmp(write_buffer, read_buffer, size);
        if (rc == 0) {
            printf("         SUCCESS: Data read back matches data written.\n");
        } else {
            fprintf(stderr, "         FAILURE: Data verification failed on cycle %lu!\n", i + 1);
            
            // Call the helper function to dump buffers for detailed comparison
            dump_buffers(write_buffer, read_buffer, size);
            
            rc = -1; // Set an error code
            goto out; // Exit immediately on error
        }
    }

    printf("\n====================================================\n");
    printf("DMA Data Verification SUCCESSFUL for all %lu cycles.\n", count);
    printf("====================================================\n");
    rc = 0;

out:
    close(fpga_fd);
    free(write_buffer);
    free(read_buffer);
    return rc;
}