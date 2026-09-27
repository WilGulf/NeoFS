#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "fs.h"
#include "path.h"
#include "global.h"

int main(int argc, char **argv) {
    char *disk_path = "";
    char *flags = "";

    char *input = "";
    char *target_path = "";

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--disk") && i + 1 < argc) {
            disk_path = argv[++i];
        }

        if (!strcmp(argv[i], "--flags") && i + 1 < argc) {
            flags = argv[++i];
        }

        if (!strncmp(argv[i], "if=", 3)) {
            input = argv[i] + 3;
        }

        if (!strncmp(argv[i], "trgt=", 5)) {
            target_path = argv[i] + 5;
        }

        if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) {
            verbose = true;
        }
    }

    FILE *file = fopen(input, "rb");
    if (!file) {
        return -1;
    }

    fseek(file, 0L, SEEK_END);
    long int res = ftell(file);
    fseek(file, 0, SEEK_SET);

    char file_buffer[res];
    memset(&file_buffer, 0, sizeof(file_buffer));
    fread(file_buffer, 1, sizeof(file_buffer), file);
    fclose(file);   

    FILE *disk = fopen(disk_path, "rb+");
    if (disk) {
        if (fs_init(disk) != 0) {
            fclose(disk);
            return -1;
        }

        int meta_block_n = get_path_meta_block(target_path, true, disk);
        if (meta_block_n < 0) {
            printf("FAILED TO OPEN FILE ON DISK\n");
            return -1;
        }

        write_file(meta_block_n, file_buffer, sizeof(file_buffer), disk);

        struct meta_block meta_block;
        get_meta_block(meta_block_n, &meta_block, disk);
        if (flags[0]) {
            char *read = strchr(flags, 'r');
            char *write = strchr(flags, 'w');
            char *exec = strchr(flags, 'x');

            if (read) {
                meta_block.flags |= FLAG_R;
            }
            if (write) {
                meta_block.flags |= FLAG_W;
            }
            if (exec) {
                meta_block.flags |= FLAG_X;
            }

            write_block(meta_block_n, &meta_block, sizeof(meta_block), disk);
        }


        fclose(disk);
    }

    return 0;
}