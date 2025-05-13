#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Constants
#define BLOCK_SIZE 4096
#define TOTAL_BLOCKS 64
#define INODE_SIZE 256
#define MAGIC_NUMBER 0xD34D
#define SUPERBLOCK_SIZE 4096
#define INODE_PER_BLOCK (BLOCK_SIZE / INODE_SIZE)
#define DATA_BLOCK_START 8
#define INODE_TABLE_START 3
#define INODE_BLOCK_COUNT 5
#define INODE_COUNT (INODE_PER_BLOCK * INODE_BLOCK_COUNT)
#define MAX_ERRORS 1000
#define MAX_INODE_LIST 200 // Reduced to fit within 256-byte error buffer

// Structures
typedef struct {
    uint16_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_block;
    uint32_t first_data_block;
    uint32_t inode_size;
    uint32_t inode_count;
    bool valid;
    char errors[MAX_ERRORS][256];
    int error_count;
} Superblock;

typedef struct {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint32_t links;
    uint32_t blocks;
    uint32_t direct;
    uint32_t indirect;
    uint32_t double_indirect;
    uint32_t triple_indirect;
    bool valid;
    char errors[MAX_ERRORS][256];
    int error_count;
} Inode;

typedef struct {
    FILE *fs_image;
    bool *inode_bitmap;
    bool *data_bitmap;
    Inode *inodes;
    uint32_t **block_references; // Array of pointers to inode lists
    uint32_t *block_ref_counts;  // Number of inodes referencing each block
    char errors[MAX_ERRORS][256];
    int error_count;
} VSFSCK;

// Function prototypes
void init_superblock(Superblock *sb, uint8_t *data);
bool validate_superblock(Superblock *sb);
void init_inode(Inode *inode, uint8_t *data);
bool is_inode_valid(Inode *inode);
void get_inode_blocks(VSFSCK *checker, Inode *inode, int inode_idx, uint32_t *blocks, int *block_count);
bool *read_bitmap(FILE *fs_image, uint32_t block_num);
void validate_inodes(VSFSCK *checker);
void validate_data_bitmap(VSFSCK *checker);
void check_duplicates(VSFSCK *checker);
void check_bad_blocks(VSFSCK *checker);
void fix_errors(VSFSCK *checker);
void run_checker(VSFSCK *checker);
void free_checker(VSFSCK *checker);

// Superblock initialization
void init_superblock(Superblock *sb, uint8_t *data) {
    sb->magic = *(uint16_t *)data;
    sb->block_size = *(uint32_t *)(data + 2);
    sb->total_blocks = *(uint32_t *)(data + 6);
    sb->inode_bitmap_block = *(uint32_t *)(data + 10);
    sb->data_bitmap_block = *(uint32_t *)(data + 14);
    sb->inode_table_block = *(uint32_t *)(data + 18);
    sb->first_data_block = *(uint32_t *)(data + 22);
    sb->inode_size = *(uint32_t *)(data + 26);
    sb->inode_count = *(uint32_t *)(data + 30);
    sb->valid = true;
    sb->error_count = 0;
}

// Validate superblock
bool validate_superblock(Superblock *sb) {
    if (sb->magic != MAGIC_NUMBER) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid magic number: 0x%04x != 0x%04x", sb->magic, MAGIC_NUMBER);
        sb->valid = false;
    }
    if (sb->block_size != BLOCK_SIZE) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid block size: %u != %u", sb->block_size, BLOCK_SIZE);
        sb->valid = false;
    }
    if (sb->total_blocks != TOTAL_BLOCKS) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid total blocks: %u != %u", sb->total_blocks, TOTAL_BLOCKS);
        sb->valid = false;
    }
    if (sb->inode_bitmap_block != 1) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid inode bitmap block: %u != 1", sb->inode_bitmap_block);
        sb->valid = false;
    }
    if (sb->data_bitmap_block != 2) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid data bitmap block: %u != 2", sb->data_bitmap_block);
        sb->valid = false;
    }
    if (sb->inode_table_block != INODE_TABLE_START) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid inode table block: %u != %u", sb->inode_table_block, INODE_TABLE_START);
        sb->valid = false;
    }
    if (sb->first_data_block != DATA_BLOCK_START) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid first data block: %u != %u", sb->first_data_block, DATA_BLOCK_START);
        sb->valid = false;
    }
    if (sb->inode_size != INODE_SIZE) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid inode size: %u != %u", sb->inode_size, INODE_SIZE);
        sb->valid = false;
    }
    if (sb->inode_count != INODE_COUNT) {
        snprintf(sb->errors[sb->error_count++], 256, "Invalid inode count: %u != %u", sb->inode_count, INODE_COUNT);
        sb->valid = false;
    }
    return sb->valid;
}

// Inode initialization
void init_inode(Inode *inode, uint8_t *data) {
    inode->mode = *(uint32_t *)data;
    inode->uid = *(uint32_t *)(data + 4);
    inode->gid = *(uint32_t *)(data + 8);
    inode->size = *(uint32_t *)(data + 12);
    inode->atime = *(uint32_t *)(data + 16);
    inode->ctime = *(uint32_t *)(data + 20);
    inode->mtime = *(uint32_t *)(data + 24);
    inode->dtime = *(uint32_t *)(data + 28);
    inode->links = *(uint32_t *)(data + 32);
    inode->blocks = *(uint32_t *)(data + 36);
    inode->direct = *(uint32_t *)(data + 40);
    inode->indirect = *(uint32_t *)(data + 44);
    inode->double_indirect = *(uint32_t *)(data + 48);
    inode->triple_indirect = *(uint32_t *)(data + 52);
    inode->valid = true;
    inode->error_count = 0;
}

// Check inode validity
bool is_inode_valid(Inode *inode) {
    return inode->links > 0 && inode->dtime == 0;
}

// Get blocks referenced by inode
void get_inode_blocks(VSFSCK *checker, Inode *inode, int inode_idx, uint32_t *blocks, int *block_count) {
    *block_count = 0;
    if (inode->direct != 0) {
        if (inode->direct < DATA_BLOCK_START || inode->direct >= TOTAL_BLOCKS) {
            snprintf(inode->errors[inode->error_count++], 256, "Invalid direct block pointer: %u", inode->direct);
            inode->valid = false;
        } else {
            blocks[(*block_count)++] = inode->direct;
            if (!checker->block_references[inode->direct]) {
                checker->block_references[inode->direct] = malloc(sizeof(uint32_t));
                if (!checker->block_references[inode->direct]) {
                    fprintf(stderr, "Memory allocation failed for block_references\n");
                    exit(1);
                }
                checker->block_ref_counts[inode->direct] = 0;
            }
            checker->block_references[inode->direct] = realloc(checker->block_references[inode->direct], 
                (checker->block_ref_counts[inode->direct] + 1) * sizeof(uint32_t));
            if (!checker->block_references[inode->direct]) {
                fprintf(stderr, "Memory reallocation failed for block_references\n");
                exit(1);
            }
            checker->block_references[inode->direct][checker->block_ref_counts[inode->direct]++] = inode_idx;
        }
    }
    if (inode->indirect != 0) {
        if (inode->indirect < DATA_BLOCK_START || inode->indirect >= TOTAL_BLOCKS) {
            snprintf(inode->errors[inode->error_count++], 256, "Invalid indirect block pointer: %u", inode->indirect);
            inode->valid = false;
        } else {
            fseek(checker->fs_image, inode->indirect * BLOCK_SIZE, SEEK_SET);
            uint32_t pointers[BLOCK_SIZE / 4];
            fread(pointers, sizeof(uint32_t), BLOCK_SIZE / 4, checker->fs_image);
            for (int i = 0; i < BLOCK_SIZE / 4; i++) {
                if (pointers[i] != 0) {
                    if (pointers[i] < DATA_BLOCK_START || pointers[i] >= TOTAL_BLOCKS) {
                        snprintf(inode->errors[inode->error_count++], 256, "Invalid indirect pointer: %u", pointers[i]);
                        inode->valid = false;
                    } else {
                        blocks[(*block_count)++] = pointers[i];
                        if (!checker->block_references[pointers[i]]) {
                            checker->block_references[pointers[i]] = malloc(sizeof(uint32_t));
                            if (!checker->block_references[pointers[i]]) {
                                fprintf(stderr, "Memory allocation failed for block_references\n");
                                exit(1);
                            }
                            checker->block_ref_counts[pointers[i]] = 0;
                        }
                        checker->block_references[pointers[i]] = realloc(checker->block_references[pointers[i]], 
                            (checker->block_ref_counts[pointers[i]] + 1) * sizeof(uint32_t));
                        if (!checker->block_references[pointers[i]]) {
                            fprintf(stderr, "Memory reallocation failed for block_references\n");
                            exit(1);
                        }
                        checker->block_references[pointers[i]][checker->block_ref_counts[pointers[i]]++] = inode_idx;
                    }
                }
            }
        }
    }
    // Double and triple indirect blocks omitted for brevity
}

// Read bitmap
bool *read_bitmap(FILE *fs_image, uint32_t block_num) {
    bool *bitmap = malloc(BLOCK_SIZE * 8 * sizeof(bool));
    if (!bitmap) {
        fprintf(stderr, "Memory allocation failed for bitmap\n");
        exit(1);
    }
    fseek(fs_image, block_num * BLOCK_SIZE, SEEK_SET);
    uint8_t buffer[BLOCK_SIZE];
    fread(buffer, 1, BLOCK_SIZE, fs_image);
    for (int i = 0; i < BLOCK_SIZE * 8; i++) {
        bitmap[i] = (buffer[i / 8] >> (i % 8)) & 1;
    }
    return bitmap;
}

// Validate inodes
void validate_inodes(VSFSCK *checker) {
    fseek(checker->fs_image, INODE_TABLE_START * BLOCK_SIZE, SEEK_SET);
    uint8_t buffer[INODE_SIZE];
    for (int i = 0; i < INODE_COUNT; i++) {
        fread(buffer, 1, INODE_SIZE, checker->fs_image);
        init_inode(&checker->inodes[i], buffer);
        if (is_inode_valid(&checker->inodes[i]) && !checker->inode_bitmap[i]) {
            snprintf(checker->errors[checker->error_count++], 256, "Inode %d is valid but not marked in bitmap", i);
        }
        if (!is_inode_valid(&checker->inodes[i]) && checker->inode_bitmap[i]) {
            snprintf(checker->errors[checker->error_count++], 256, "Inode %d is invalid but marked in bitmap", i);
        }
        if (is_inode_valid(&checker->inodes[i])) {
            uint32_t blocks[1024];
            int block_count;
            get_inode_blocks(checker, &checker->inodes[i], i, blocks, &block_count);
        }
    }
}

// Validate data bitmap
void validate_data_bitmap(VSFSCK *checker) {
    for (uint32_t block = DATA_BLOCK_START; block < TOTAL_BLOCKS; block++) {
        bool referenced = checker->block_ref_counts[block] > 0;
        bool marked = checker->data_bitmap[block - DATA_BLOCK_START];
        if (referenced && !marked) {
            snprintf(checker->errors[checker->error_count++], 256, "Block %u is referenced but not marked in data bitmap", block);
        }
        if (!referenced && marked) {
            snprintf(checker->errors[checker->error_count++], 256, "Block %u is marked in data bitmap but not referenced", block);
        }
    }
}

// Check for duplicate blocks
void check_duplicates(VSFSCK *checker) {
    for (uint32_t block = DATA_BLOCK_START; block < TOTAL_BLOCKS; block++) {
        if (checker->block_ref_counts[block] > 1) {
            char inode_list[MAX_INODE_LIST] = "";
            int len = 0;
            for (uint32_t i = 0; i < checker->block_ref_counts[block]; i++) {
                int remaining = MAX_INODE_LIST - len - 1;
                if (remaining <= 0) {
                    // Truncate the list with "..." to indicate more inodes
                    strncpy(inode_list + len - 3, "...", 4);
                    break;
                }
                int written = snprintf(inode_list + len, remaining, "%u,", checker->block_references[block][i]);
                if (written >= remaining) {
                    strncpy(inode_list + len - 3, "...", 4);
                    break;
                }
                len += written;
            }
            if (len > 0 && inode_list[len - 1] == ',') inode_list[len - 1] = '\0';
            snprintf(checker->errors[checker->error_count++], 256, "Block %u referenced by multiple inodes: %s", block, inode_list);
        }
    }
}

// Check for bad blocks
void check_bad_blocks(VSFSCK *checker) {
    for (uint32_t block = 0; block < TOTAL_BLOCKS; block++) {
        if (checker->block_ref_counts[block] > 0 && (block < DATA_BLOCK_START || block >= TOTAL_BLOCKS)) {
            char inode_list[MAX_INODE_LIST] = "";
            int len = 0;
            for (uint32_t i = 0; i < checker->block_ref_counts[block]; i++) {
                int remaining = MAX_INODE_LIST - len - 1;
                if (remaining <= 0) {
                    // Truncate the list with "..." to indicate more inodes
                    strncpy(inode_list + len - 3, "...", 4);
                    break;
                }
                int written = snprintf(inode_list + len, remaining, "%u,", checker->block_references[block][i]);
                if (written >= remaining) {
                    strncpy(inode_list + len - 3, "...", 4);
                    break;
                }
                len += written;
            }
            if (len > 0 && inode_list[len - 1] == ',') inode_list[len - 1] = '\0';
            snprintf(checker->errors[checker->error_count++], 256, "Invalid block number %u referenced by inodes: %s", block, inode_list);
        }
    }
}

// Fix errors
void fix_errors(VSFSCK *checker) {
    // Fix superblock
    fseek(checker->fs_image, 0, SEEK_SET);
    uint8_t superblock_data[36] = {0};
    *(uint16_t *)superblock_data = MAGIC_NUMBER;
    *(uint32_t *)(superblock_data + 2) = BLOCK_SIZE;
    *(uint32_t *)(superblock_data + 6) = TOTAL_BLOCKS;
    *(uint32_t *)(superblock_data + 10) = 1;
    *(uint32_t *)(superblock_data + 14) = 2;
    *(uint32_t *)(superblock_data + 18) = INODE_TABLE_START;
    *(uint32_t *)(superblock_data + 22) = DATA_BLOCK_START;
    *(uint32_t *)(superblock_data + 26) = INODE_SIZE;
    *(uint32_t *)(superblock_data + 30) = INODE_COUNT;
    fwrite(superblock_data, 1, 36, checker->fs_image);
    printf("Fixed superblock\n");

    // Fix inode bitmap
    uint8_t inode_bitmap_data[BLOCK_SIZE] = {0};
    for (int i = 0; i < INODE_COUNT; i++) {
        if (is_inode_valid(&checker->inodes[i])) {
            inode_bitmap_data[i / 8] |= (1 << (i % 8));
        }
    }
    fseek(checker->fs_image, BLOCK_SIZE, SEEK_SET);
    fwrite(inode_bitmap_data, 1, BLOCK_SIZE, checker->fs_image);
    printf("Fixed inode bitmap\n");

    // Fix data bitmap
    uint8_t data_bitmap_data[BLOCK_SIZE] = {0};
    for (uint32_t block = DATA_BLOCK_START; block < TOTAL_BLOCKS; block++) {
        if (checker->block_ref_counts[block] > 0) {
            uint32_t idx = block - DATA_BLOCK_START;
            data_bitmap_data[idx / 8] |= (1 << (idx % 8));
        }
    }
    fseek(checker->fs_image, 2 * BLOCK_SIZE, SEEK_SET);
    fwrite(data_bitmap_data, 1, BLOCK_SIZE, checker->fs_image);
    printf("Fixed data bitmap\n");

    // Clear invalid blocks in inodes
    for (int i = 0; i < INODE_COUNT; i++) {
        if (!is_inode_valid(&checker->inodes[i])) continue;
        uint32_t blocks[1024];
        int block_count;
        get_inode_blocks(checker, &checker->inodes[i], i, blocks, &block_count);
        bool has_invalid = false;
        for (int j = 0; j < block_count; j++) {
            if (blocks[j] < DATA_BLOCK_START || blocks[j] >= TOTAL_BLOCKS) {
                has_invalid = true;
                break;
            }
        }
        if (has_invalid) {
            fseek(checker->fs_image, INODE_TABLE_START * BLOCK_SIZE + i * INODE_SIZE, SEEK_SET);
            uint8_t inode_data[INODE_SIZE] = {0};
            memcpy(inode_data, &checker->inodes[i], 56); // Preserve first 56 bytes
            fwrite(inode_data, 1, INODE_SIZE, checker->fs_image);
            printf("Fixed invalid blocks in inode %d\n", i);
        }
    }
    fflush(checker->fs_image);
}

// Run the checker
void run_checker(VSFSCK *checker) {
    printf("Starting VSFS consistency check...\n");
    checker->inode_bitmap = read_bitmap(checker->fs_image, 1);
    checker->data_bitmap = read_bitmap(checker->fs_image, 2);

    uint8_t superblock_data[SUPERBLOCK_SIZE];
    fseek(checker->fs_image, 0, SEEK_SET);
    fread(superblock_data, 1, SUPERBLOCK_SIZE, checker->fs_image);
    Superblock sb;
    init_superblock(&sb, superblock_data);
    if (!validate_superblock(&sb)) {
        for (int i = 0; i < sb.error_count; i++) {
            snprintf(checker->errors[checker->error_count++], 256, "%s", sb.errors[i]);
        }
    }

    validate_inodes(checker);
    validate_data_bitmap(checker);
    check_duplicates(checker);
    check_bad_blocks(checker);

    if (checker->error_count > 0) {
        printf("Errors found:\n");
        for (int i = 0; i < checker->error_count; i++) {
            printf("- %s\n", checker->errors[i]);
        }
        printf("\nAttempting to fix errors...\n");
        fix_errors(checker);
        printf("Fixes applied. Please re-run vsfsck to verify.\n");
    } else {
        printf("No errors found. File system is consistent.\n");
    }
}

// Free resources
void free_checker(VSFSCK *checker) {
    free(checker->inode_bitmap);
    free(checker->data_bitmap);
    free(checker->inodes);
    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        free(checker->block_references[i]);
    }
    free(checker->block_references);
    free(checker->block_ref_counts);
    fclose(checker->fs_image);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_path>\n", argv[0]);
        return 1;
    }

    FILE *fs_image = fopen(argv[1], "r+b");
    if (!fs_image) {
        perror("Failed to open image file");
        return 1;
    }

    VSFSCK checker = {0};
    checker.fs_image = fs_image;
    checker.inodes = malloc(INODE_COUNT * sizeof(Inode));
    if (!checker.inodes) {
        fprintf(stderr, "Memory allocation failed for inodes\n");
        fclose(fs_image);
        return 1;
    }
    checker.block_references = malloc(TOTAL_BLOCKS * sizeof(uint32_t *));
    if (!checker.block_references) {
        fprintf(stderr, "Memory allocation failed for block_references\n");
        free(checker.inodes);
        fclose(fs_image);
        return 1;
    }
    checker.block_ref_counts = calloc(TOTAL_BLOCKS, sizeof(uint32_t));
    if (!checker.block_ref_counts) {
        fprintf(stderr, "Memory allocation failed for block_ref_counts\n");
        free(checker.inodes);
        free(checker.block_references);
        fclose(fs_image);
        return 1;
    }
    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        checker.block_references[i] = NULL;
    }

    run_checker(&checker);      
    free_checker(&checker);
    return 0;
}