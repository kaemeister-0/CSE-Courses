#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define BLOCK_SIZE 4096
#define TOTAL_BLOCKS 64
#define INODE_SIZE 256
#define MAGIC_NUMBER 0xD34D
#define INODE_COUNT (5 * BLOCK_SIZE / INODE_SIZE)
#define EXPECTED_SIZE (TOTAL_BLOCKS * BLOCK_SIZE)

typedef struct {
    uint16_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_start;
    uint32_t first_data_block;
    uint32_t inode_size;
    uint32_t inode_count;
    uint8_t reserved[4058];
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
    uint32_t direct[1];
    uint32_t single_indirect;
    uint32_t double_indirect;
    uint32_t triple_indirect;
    uint8_t reserved[156];
} Inode;

typedef struct {
    int fd;
    void *image;
    size_t size;
    Superblock *sb;
    uint8_t *inode_bitmap;
    uint8_t *data_bitmap;
    Inode *inodes;
    uint32_t *data_blocks;
} VSFS;

int validate_superblock(VSFS *fs) {
    int errors = 0;
    if (fs->sb->magic != MAGIC_NUMBER) {
        printf("Superblock: Invalid magic number (expected 0x%X, got 0x%X)\n", MAGIC_NUMBER, fs->sb->magic);
        errors++;
    }
    if (fs->sb->block_size != BLOCK_SIZE) {
        printf("Superblock: Invalid block size (expected %d, got %d)\n", BLOCK_SIZE, fs->sb->block_size);
        errors++;
    }
    if (fs->sb->total_blocks != TOTAL_BLOCKS) {
        printf("Superblock: Invalid total blocks (expected %d, got %d)\n", TOTAL_BLOCKS, fs->sb->total_blocks);
        errors++;
    }
    if (fs->sb->inode_bitmap_block != 1) {
        printf("Superblock: Invalid inode bitmap block (expected 1, got %d)\n", fs->sb->inode_bitmap_block);
        errors++;
    }
    if (fs->sb->data_bitmap_block != 2) {
        printf("Superblock: Invalid data bitmap block (expected 2, got %d)\n", fs->sb->data_bitmap_block);
        errors++;
    }
    if (fs->sb->inode_table_start != 3) {
        printf("Superblock: Invalid inode table start (expected 3, got %d)\n", fs->sb->inode_table_start);
        errors++;
    }
    if (fs->sb->first_data_block != 8) {
        printf("Superblock: Invalid first data block (expected 8, got %d)\n", fs->sb->first_data_block);
        errors++;
    }
    if (fs->sb->inode_size != INODE_SIZE) {
        printf("Superblock: Invalid inode size (expected %d, got %d)\n", INODE_SIZE, fs->sb->inode_size);
        errors++;
    }
    if (fs->sb->inode_count != INODE_COUNT) {
        printf("Superblock: Invalid inode count (expected %d, got %d)\n", INODE_COUNT, fs->sb->inode_count);
        errors++;
    }
    return errors == 0 ? 0 : 1; // Return 0 for no errors, 1 for errors
}

int check_data_bitmap(VSFS *fs, uint32_t *used_blocks) {
    uint8_t temp_bitmap[BLOCK_SIZE] = {0};
    int errors = 0;
    
    for (int i = 0; i < INODE_COUNT; i++) {
        Inode *inode = &fs->inodes[i];
        if (inode->links > 0 && inode->dtime == 0) {
            if (inode->direct[0] >= TOTAL_BLOCKS || inode->direct[0] < fs->sb->first_data_block) {
                printf("Inode %d: Skipping invalid block %d\n", i, inode->direct[0]);
                continue;
            }
            temp_bitmap[(inode->direct[0] - fs->sb->first_data_block) / 8] |= 
                (1 << ((inode->direct[0] - fs->sb->first_data_block) % 8));
            used_blocks[inode->direct[0]]++;
        }
    }

    for (int i = 0; i < TOTAL_BLOCKS - fs->sb->first_data_block; i++) {
        int bit = (fs->data_bitmap[i/8] >> (i%8)) & 1;
        if (bit && !(temp_bitmap[i/8] & (1 << (i%8)))) {
            printf("Data block %d marked used but not referenced\n", i + fs->sb->first_data_block);
            errors++;
        }
        if (!bit && (temp_bitmap[i/8] & (1 << (i%8)))) {
            printf("Data block %d referenced but not marked used\n", i + fs->sb->first_data_block);
            errors++;
            fs->data_bitmap[i/8] |= (1 << (i%8));
        }
    }
    return errors;
}

int check_inode_bitmap(VSFS *fs) {
    uint8_t temp_bitmap[BLOCK_SIZE] = {0};
    int errors = 0;

    for (int i = 0; i < INODE_COUNT; i++) {
        if (fs->inodes[i].links > 0 && fs->inodes[i].dtime == 0) {
            temp_bitmap[i/8] |= (1 << (i%8));
        }
    }

    for (int i = 0; i < INODE_COUNT; i++) {
        int bit = (fs->inode_bitmap[i/8] >> (i%8)) & 1;
        if (bit && !(temp_bitmap[i/8] & (1 << (i%8)))) {
            printf("Inode %d marked used but invalid\n", i);
            errors++;
            fs->inode_bitmap[i/8] &= ~(1 << (i%8));
        }
        if (!bit && (temp_bitmap[i/8] & (1 << (i%8)))) {
            printf("Inode %d valid but not marked used\n", i);
            errors++;
            fs->inode_bitmap[i/8] |= (1 << (i%8));
        }
    }
    return errors;
}

int check_duplicates(VSFS *fs, uint32_t *used_blocks) {
    int errors = 0;
    for (int i = fs->sb->first_data_block; i < TOTAL_BLOCKS; i++) {
        if (used_blocks[i] > 1) {
            printf("Block %d referenced %d times\n", i, used_blocks[i]);
            errors++;
        }
    }
    return errors;
}

int check_bad_blocks(VSFS *fs) {
    int errors = 0;
    for (int i = 0; i < INODE_COUNT; i++) {
        if (fs->inodes[i].links > 0 && fs->inodes[i].dtime == 0) {
            if (fs->inodes[i].direct[0] >= TOTAL_BLOCKS || 
                fs->inodes[i].direct[0] < fs->sb->first_data_block) {
                printf("Inode %d references invalid block %d\n", i, fs->inodes[i].direct[0]);
                errors++;
                fs->inodes[i].direct[0] = 0;
            }
        }
    }
    return errors;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_file>\n", argv[0]);
        return 1;
    }

    VSFS fs;
    fs.fd = open(argv[1], O_RDWR);
    if (fs.fd < 0) {
        perror("Failed to open image");
        fprintf(stderr, "Ensure '%s' exists in the current directory or provide the correct path.\n", argv[1]);
        return 1;
    }

    struct stat st;
    if (fstat(fs.fd, &st) < 0) {
        perror("Failed to stat image");
        close(fs.fd);
        return 1;
    }
    fs.size = st.st_size;
    if (fs.size < EXPECTED_SIZE) {
        fprintf(stderr, "Image size too small: %zu bytes (expected at least %d bytes)\n", fs.size, EXPECTED_SIZE);
        close(fs.fd);
        return 1;
    }

    fs.image = mmap(NULL, fs.size, PROT_READ | PROT_WRITE, MAP_SHARED, fs.fd, 0);
    if (fs.image == MAP_FAILED) {
        perror("Failed to mmap image");
        close(fs.fd);
        return 1;
    }

    fs.sb = (Superblock *)fs.image;
    fs.inode_bitmap = (uint8_t *)(fs.image + BLOCK_SIZE);
    fs.data_bitmap = (uint8_t *)(fs.image + 2 * BLOCK_SIZE);
    fs.inodes = (Inode *)(fs.image + 3 * BLOCK_SIZE);
    fs.data_blocks = (uint32_t *)(fs.image + 8 * BLOCK_SIZE);

    uint32_t used_blocks[TOTAL_BLOCKS] = {0};
    int errors = 0;

    printf("Checking superblock...\n");
    errors += validate_superblock(&fs);
    printf("Checking data bitmap...\n");
    errors += check_data_bitmap(&fs, used_blocks);
    printf("Checking inode bitmap...\n");
    errors += check_inode_bitmap(&fs);
    printf("Checking for duplicate blocks...\n");
    errors += check_duplicates(&fs, used_blocks);
    printf("Checking for bad blocks...\n");
    errors += check_bad_blocks(&fs);

    printf("Total errors found and fixed: %d\n", errors);

    if (msync(fs.image, fs.size, MS_SYNC) < 0) {
        perror("Failed to sync image");
    }
    munmap(fs.image, fs.size);
    close(fs.fd);
    return 0;
}
