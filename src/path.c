#include "path.h"
#include "fs.h"

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

struct path_part *string_to_path(char *path) {
    struct path_part *path_part = malloc(sizeof(struct path_part));

    struct path_part *curr = 0;
    struct path_part *prev = 0;

    char *p = strtok(path, "/");

    path_part->part = p;
    path_part->next = 0;
    prev = path_part;
    p = strtok(NULL, "/");
    while (p != NULL) {
        curr = malloc(sizeof(struct path_part));
        curr->part = p;
        curr->next = 0;
        prev->next = curr;
        prev = curr;

        p = strtok(NULL, "/");
    }

    return path_part;
}

void free_path_part(struct path_part *path_part) {
    struct path_part *part = path_part;
    while (part) {
        struct path_part *next_part = part->next;
        free(part); 
        part = next_part;
    }
}

static int name_matches(struct meta_block *meta_block, const char *part) {
    char buffer[strlen(part) + 1];
    strcpy(buffer, part);

    char *dot = strrchr(buffer, '.');
    if (dot != 0) {
        *dot = '\0';

        char *filename = buffer;
        char *ext = dot + 1;

        if (strncmp(filename, meta_block->filename, sizeof(meta_block->filename))) {
            return false;
        }
        if (strncmp(ext, meta_block->ext, sizeof(meta_block->ext))) {
            return false;
        }

        return true;
    } else {
        if (strncmp(buffer, meta_block->filename, sizeof(meta_block->filename))) {
            return false;
        }

        return true;
    }
}

int create_meta_block(int parent, bool is_dir, const char *name, FILE *disk) {
    int new_block_n = get_free_block(1, disk);
    if (new_block_n < 0) {
        return -1;
    }

    struct meta_block new_block;
    memset(new_block.filename, 0, sizeof(new_block.filename));
    memset(new_block.ext, 0, sizeof(new_block.ext));

    char buffer[strlen(name) + 1];
    strcpy(buffer, name);
    char *dot = strrchr(buffer, '.');
    if (dot != 0) {
        *dot = '\0';

        char *ext = dot + 1;
        size_t ext_len = (strlen(ext) > sizeof(new_block.ext)) ? sizeof(new_block.ext) : strlen(ext);
        strncpy(new_block.ext, ext, ext_len);
    }

    char *filename = buffer;
    size_t filename_len = (strlen(filename) > sizeof(new_block.filename)) ? sizeof(new_block.filename) : strlen(filename);
    strncpy(new_block.filename, filename, filename_len);

    new_block.is_dir = is_dir;
    new_block.next = 0;
    new_block.parent = parent;
    new_block.size = 0;
    new_block.start = 0;
    new_block.flags = 0;
    set_block_on_bitmap(new_block_n, true, disk);

    write_block(new_block_n, &new_block, sizeof(new_block), disk);

    struct meta_block parent_block;
    get_meta_block(parent, &parent_block, disk);
    
    if (!parent_block.start) {
        parent_block.start = new_block_n;
        write_block(parent, &parent_block, sizeof(parent_block), disk);
    } else {
        struct meta_block child_block;
        get_meta_block(parent_block.start, &child_block, disk);

        int prev_child_block_n = parent_block.start;
        while (child_block.next) {
            prev_child_block_n = child_block.next;
            get_meta_block(child_block.next, &child_block, disk);
        }

        get_meta_block(prev_child_block_n, &child_block, disk);
        child_block.next = new_block_n;
        write_block(prev_child_block_n, &child_block, sizeof(child_block), disk);
    }

    return new_block_n;
}

int get_path_meta_block(char *path, bool allow_creation, FILE *disk) {
    int res = 0;

    struct master_block master_block;
    get_master_block(&master_block, disk);
    
    struct meta_block root;
    get_meta_block(master_block.first_block, &root, disk);
    
    struct path_part *path_part = string_to_path(path);
    if (!path_part->part) {
        res = -1;
        goto out;
    }

    struct meta_block curr;
    struct meta_block dir;

    int dir_n = master_block.first_block; // Dir being searched
    int curr_n = root.start; // Inspected file in dir

    get_meta_block(master_block.first_block, &dir, disk);

    if (dir.start) {
        curr_n = dir.start;
        get_meta_block(curr_n, &curr, disk);
    }

    while (path_part->part) {
        bool found = false;
        while (1) {
            if (!dir.start) {
                break;
            }

            if (name_matches(&curr, path_part->part)) {
                found = true;
                break;
            }

            if (!curr.next) {
                break;
            }

            curr_n = curr.next;
            get_meta_block(curr_n, &curr, disk);
        }

        if (found && path_part->next) {
            if (!curr.is_dir) {
                res = -1;
                goto out;
            }

            dir_n = curr_n; // Jump into new dir
            get_meta_block(dir_n, &dir, disk);

            curr_n = dir.start; // Start next search on first child in new dir
            if (curr_n) {
                get_meta_block(curr_n, &curr, disk);
            }

            path_part = path_part->next; // Go to the next path_part
            continue;
        }

        if (found && !path_part->next) {
            res = curr_n; // File found
            goto out;
        }

        if (!found && path_part->next) {
            if (!allow_creation) {
                res = -1;
                goto out;
            }

            int new_dir = create_meta_block(dir_n, true, path_part->part, disk);
            
            dir_n = new_dir;
            get_meta_block(dir_n, &dir, disk);
            curr_n = dir.start;
            path_part = path_part->next;
            continue;
        }

        if (!found && !path_part->next) {
            if (!allow_creation) {
                res = -1;
                goto out;
            }

            int new_file = create_meta_block(dir_n, false, path_part->part, disk);
            res = new_file;
            goto out;
        }
    }

out:
    free_path_part(path_part);
    return res;
}