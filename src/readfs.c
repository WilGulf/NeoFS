#include "fs.h"
#include "path.h"
#include "global.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

int main(int argc, char **argv) {
    char *disk_path = "";
    char *target_path = "";

    bool disk_found = false;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--disk") && i + 1 < argc) {
            disk_path = argv[++i];
            disk_found = true;
        }

        if (disk_found && !strstr(argv[i], "--")) {
            target_path = argv[i];
        }

        if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v")) {
            verbose = true;
        }
    }

    if (disk_path[0] && target_path[0]) {
        FILE *disk = fopen(disk_path, "rb");
        if (!disk) {
            return -1;
        }

        if (fs_init(disk) != 0) {
            fclose(disk);
            return -1;
        }

        int meta_block = get_path_meta_block(target_path, false, disk);
        if (meta_block < 0) {
            return -1;
        }
        
        struct meta_block file_meta_block;
        get_meta_block(meta_block, &file_meta_block, disk);
        char buffer[file_meta_block.size + 1];
        read_file(&file_meta_block, buffer, disk);

        buffer[sizeof(buffer) - 1] = 0x00;

        printf("%s", buffer);
    }

    return 0;
}