#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <memory.h>

#include "fs.h"

#define DEFAULT_BLOCK_SIZE 1024

#define DISK_SIZE (1024 * 1024)

int main(int argc, char **argv) {
    char *disk_path = "";
    uint32_t target_block_size = DEFAULT_BLOCK_SIZE;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--disk") && i + 1 < argc) {
            disk_path = argv[++i];
        }

        if (!strcmp(argv[i], "--block_size") && i + 1 < argc) {
            target_block_size = atoi(argv[++i]);
        }
    }

    FILE *disk = fopen(disk_path, "rb+");
    if (!disk) {
        return -1;
    }

    fseek(disk, DISK_SIZE - 1, SEEK_SET);
    fputc(0, disk);

    BLOCK_SIZE = target_block_size;
    DATA_SIZE = BLOCK_SIZE - BLOCK_DATA_OFFSET;
    uint32_t total_blocks =  DISK_SIZE / BLOCK_SIZE;

    struct master_block block_1;

    block_1.first_block = 1;
    block_1.magic = NEOFS_MAGIC;
    block_1.block_count = total_blocks;
    printf("BLOCKS: %d\n", total_blocks);
    block_1.block_size = BLOCK_SIZE;
    block_1.bitmap_metablock = 1;

    fseek(disk, 0, SEEK_SET);
    fwrite(&block_1, 1, sizeof(block_1), disk);

    struct meta_block bitmap;
    memset(bitmap.filename, 0, sizeof(bitmap.filename));
    memset(bitmap.ext, 0, sizeof(bitmap.ext));
    strcpy(bitmap.filename, "bitmap");
    
    bitmap.is_dir = 0;
    bitmap.next = 0;
    bitmap.size = (total_blocks + 7) / 8;
    bitmap.flags = 0;
    bitmap.start = 2;
    bitmap.parent = 0;

    write_block(1, &bitmap, sizeof(bitmap), disk);

    struct block bitmap_block;
    memset(&bitmap_block, 0, sizeof(bitmap_block));
    write_block(2, &bitmap_block, sizeof(bitmap_block), disk);

    set_block_on_bitmap(0, true, disk);
    set_block_on_bitmap(1, true, disk);
    set_block_on_bitmap(2, true, disk);

    struct meta_block root;
    int root_n = get_free_block(1, disk);
    memset(root.filename, 0, sizeof(root.filename));
    memset(root.ext, 0, sizeof(root.ext));
    strcpy(root.filename, "root");
    root.is_dir = 1;

    root.next = 0;
    root.size = 0;
    root.flags = 0;
    root.start = 0;
    root.parent = 0;

    write_block(root_n, &root, sizeof(root), disk);
    set_block_on_bitmap(root_n, true, disk);

    block_1.first_block = root_n;
    fseek(disk, 0, SEEK_SET);
    fwrite(&block_1, 1, sizeof(block_1), disk);

    fclose(disk);

    return 0;
}