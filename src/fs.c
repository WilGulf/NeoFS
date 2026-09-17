#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>

uint32_t BLOCK_SIZE = 0;
uint32_t DATA_SIZE = 0;

int get_master_block(struct master_block *out, FILE *disk) {
    fseek(disk, 0, SEEK_SET);
    fread(out, 1, sizeof(struct master_block), disk);
    return 0;
}

int get_meta_block(uint32_t block, struct meta_block *out, FILE *disk) {
    fseek(disk, (BLOCK_SIZE * block), SEEK_SET);
    fread(out, 1, sizeof(struct meta_block), disk);
    return 0;
}

int get_block(uint32_t block, struct block *out, FILE *disk) {
    fseek(disk, (BLOCK_SIZE * block), SEEK_SET);
    fread(out, 1, sizeof(struct block), disk);
    return 0;
}

uint32_t get_next_block(uint32_t block, FILE *disk) {
    struct block read_block;
    fseek(disk, (BLOCK_SIZE * block), SEEK_SET);
    fread(&read_block, 1, sizeof(read_block), disk);
    return read_block.next;
}

uint32_t get_next_neighbour(uint32_t block, FILE *disk) {
    struct meta_block read_block;
    fseek(disk, (BLOCK_SIZE * block), SEEK_SET);
    fread(&read_block, 1, sizeof(read_block), disk);
    return read_block.next;
}

int get_free_block(uint32_t start, FILE *disk) {
    struct master_block master_block;
    get_master_block(&master_block, disk);
    struct meta_block bitmap;
    get_meta_block(master_block.bitmap_metablock, &bitmap, disk);

    unsigned char *buffer = malloc(bitmap.size);
    read_file(&bitmap, buffer, disk);

    for (uint32_t bit = start; bit < master_block.block_count; bit++) {
        unsigned char mask = 1 << (bit % 8);
        if (!(buffer[bit / 8] & mask)) {
            free(buffer);
            return bit;
        }
    }

    free(buffer);
    
    return -1;
}

int set_block_on_bitmap(uint32_t block, bool state, FILE *disk) {
    struct master_block master_block;
    get_master_block(&master_block, disk);
    struct meta_block bitmap;
    get_meta_block(master_block.bitmap_metablock, &bitmap, disk);

    unsigned char *buffer = malloc(bitmap.size);
    read_file(&bitmap, buffer, disk);

    unsigned char mask = 1 << (block % 8);
    if (state) {
        buffer[block / 8] |= mask;
    } else {
        buffer[block / 8] &= ~mask;
    }

    write_file(master_block.bitmap_metablock, buffer, bitmap.size, disk);

    free(buffer);

    return 0;
}

int write_block(uint32_t block, void *val, size_t total, FILE *disk) {
    fseek(disk, (BLOCK_SIZE * block), SEEK_SET);
    fwrite(val, 1, total, disk);
    return BLOCK_SIZE;
}

int read_block(uint32_t block, void *out, FILE *disk) {
    int pos = (BLOCK_SIZE * block) + BLOCK_DATA_OFFSET;
    int to_read = BLOCK_SIZE - BLOCK_DATA_OFFSET;

    fseek(disk, pos, SEEK_SET);
    fread(out, 1, to_read, disk);
    return to_read;
}

int remove_block(uint32_t block, FILE *disk) {
    struct block empty;
    memset(&empty, 0, sizeof(empty));
    write_block(block, &empty, sizeof(empty), disk);
    set_block_on_bitmap(block, false, disk);
    return 0;
}

int remove_meta_block(uint32_t block, FILE *disk) {
    struct meta_block empty;
    memset(&empty, 0, sizeof(empty));
    write_block(block, &empty, sizeof(empty), disk);
    set_block_on_bitmap(block, false, disk);
    return 0;
}

int fs_init(FILE *disk) {
    struct master_block master;
    get_master_block(&master, disk);

    if (master.magic != NEOFS_MAGIC) {
        return -1;
    }

    if (!master.block_size) {
        return -1;
    }

    BLOCK_SIZE = master.block_size;
    DATA_SIZE = master.block_size - BLOCK_DATA_OFFSET;

    return 0;
}

int read_file(struct meta_block *file_meta_block, void *out, FILE *disk) {
    int block = file_meta_block->start;
    size_t remaining = file_meta_block->size;
    int total_to_read = 0;

    while (block && remaining > 0) {
        size_t to_read = (remaining > DATA_SIZE) ? DATA_SIZE : remaining;
        char buffer[DATA_SIZE];
        int res = read_block(block, buffer, disk);
        memcpy(out, buffer, to_read);

        out += to_read;
        remaining -= to_read;
        total_to_read += to_read;
        block = get_next_block(block, disk);;
    }

    return total_to_read;
}

int write_file(uint32_t file_meta_block_n, void *in, size_t total, FILE *disk) {
    struct meta_block file_meta_block;
    get_meta_block(file_meta_block_n, &file_meta_block, disk);
    int block = file_meta_block.start;
    if (!block) {
        int new_block_n = get_free_block(1, disk);
        if (new_block_n < 0) {
            return -1;
        }

        struct block new_block;
        /*new_block.status = BLOCK_STATUS_USED;*/ set_block_on_bitmap(new_block_n, true, disk);
        new_block.next = 0;
        write_block(new_block_n, &new_block, sizeof(new_block), disk);

        file_meta_block.start = new_block_n;
        write_block(file_meta_block_n, &file_meta_block, sizeof(file_meta_block), disk);
        block = file_meta_block.start;
    }

    int blocks_written = 0;

    int blocks_needed = total / DATA_SIZE;
    if (total % DATA_SIZE) {
        blocks_needed++;
    }

    int curr = block;
    int prev = curr;
    int blocks_allocated = 1;
    while (blocks_allocated < blocks_needed) {
        curr = get_next_block(curr, disk);
        if (!curr) {
            int new_block_n = get_free_block(1, disk);
            if (new_block_n < 0) {
                return -1;
            }

            struct block new_block;
            /*new_block.status = BLOCK_STATUS_USED;*/set_block_on_bitmap(new_block_n, true, disk);
            new_block.next = 0;
            write_block(new_block_n, &new_block, sizeof(new_block), disk);

            struct block prev_block;
            get_block(prev, &prev_block, disk);
            prev_block.next = new_block_n;
            curr = new_block_n;
            write_block(prev, &prev_block, sizeof(prev_block), disk);
        }

        prev = curr;
        blocks_allocated++;
    }

    size_t remaining = total;
    curr = block;
    while (curr) {
        char buffer[BLOCK_SIZE];
        struct block curr_block;
        get_block(curr, &curr_block, disk);
        memset(buffer, 0, sizeof(buffer));
        memcpy(&buffer, &curr_block, sizeof(curr_block));

        size_t to_copy = (remaining > DATA_SIZE) ? DATA_SIZE : remaining;
        memcpy(buffer + sizeof(curr_block), in, to_copy);
        write_block(curr, buffer, BLOCK_SIZE, disk);
        curr = curr_block.next;
        in += to_copy;
        remaining -= to_copy;
        blocks_written++;
    }

    file_meta_block.size = total;
    write_block(file_meta_block_n, &file_meta_block, sizeof(file_meta_block), disk);

    return total;
}

int remove_file(uint32_t file_meta_block_n, FILE *disk) {
    if (file_meta_block_n == 1) {
        return -1;
    }

    struct meta_block file_meta_block;
    get_meta_block(file_meta_block_n, &file_meta_block, disk);

    struct meta_block parent;
    get_meta_block(file_meta_block.parent, &parent, disk);
    if (parent.start == file_meta_block_n) {
        parent.start = file_meta_block.next;
        write_block(file_meta_block.parent, &parent, sizeof(parent), disk);
    } else {
        struct meta_block child;
        int child_n = parent.start;
        get_meta_block(child_n, &child, disk);

        bool found = false;
        while (1) {
            if (child.next == file_meta_block_n) {
                child.next = file_meta_block.next;
                write_block(child_n, &child, sizeof(child), disk);
                found = true;
                break;
            }

            if (!child.next) {
                break;
            }

            child_n = child.next;
            get_meta_block(child_n, &child, disk);
        }

        if (!found) {
            return -1;
        }
    }

    if (!file_meta_block.start) {
        remove_meta_block(file_meta_block_n, disk);
        return 0;
    }

    if (file_meta_block.is_dir) {
        struct meta_block child;
        int child_n = file_meta_block.start;
        get_meta_block(file_meta_block.start, &child, disk);
        while (child.next) {
            int next = child.next;
            remove_file(child_n, disk);
            get_meta_block(next, &child, disk);
            child_n = next;
        }

        remove_file(child_n, disk);

        remove_meta_block(file_meta_block_n, disk);
    } else {
        struct block block;
        int block_n = file_meta_block.start;
        get_block(file_meta_block.start, &block, disk);
        while (block.next) {
            int next = block.next;
            remove_block(block_n, disk);
            get_block(next, &block, disk);
            block_n = next;
        }

        remove_block(block_n, disk);

        remove_meta_block(file_meta_block_n, disk);
    }

    return 0;
}