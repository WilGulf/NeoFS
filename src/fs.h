#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define NEOFS_MAGIC 0x4E454F46

#define MAX_PATH 256

#define BLOCK_STATUS_FREE 0x00
#define BLOCK_STATUS_USED 0x01

#define FLAG_R 0b00000001
#define FLAG_W 0b00000010
#define FLAG_X 0b00000100
#define FLAG_4 0b00001000
#define FLAG_5 0b00010000
#define FLAG_6 0b00100000
#define FLAG_7 0b01000000
#define FLAG_8 0b10000000

struct master_block {
    uint32_t magic;

    uint32_t block_size;

    uint32_t bitmap_metablock;

    uint32_t first_block;
    uint32_t block_count;
} __attribute__((packed));

struct meta_block {
    char filename[8];
    char ext[3];

    uint8_t is_dir;
    uint8_t flags;
    uint32_t size;

    uint32_t start;
    uint32_t next;
    uint32_t parent;
} __attribute__((packed));

struct block {
    uint32_t next;
} __attribute__((packed));

struct path_root {
    char drive_id;
    struct path_part *first;
};

struct path_part {
    const char *part;
    struct path_part *next;
};

#define BLOCK_DATA_OFFSET (sizeof(struct block))
extern uint32_t BLOCK_SIZE;
extern uint32_t DATA_SIZE;

int fs_init(FILE *disk);

int get_master_block(struct master_block *out, FILE *disk);
int get_meta_block(uint32_t block, struct meta_block *out, FILE *disk);
int get_block(uint32_t block, struct block *out, FILE *disk);

int set_block_on_bitmap(uint32_t block, bool state, FILE *disk);

uint32_t get_next_block(uint32_t block, FILE *disk);
uint32_t get_next_neighbour(uint32_t block, FILE *disk);

int get_free_block(uint32_t start, FILE *disk);

int write_block(uint32_t block, void *val, size_t total, FILE *disk);
int read_block(uint32_t block, void *out, FILE *disk);

int read_file(struct meta_block *file_meta_block, void *out, FILE *disk);
int write_file(uint32_t file_meta_block_n, void *in, size_t total, FILE *disk);
int remove_file(uint32_t file_meta_block_n, FILE *disk);

#endif