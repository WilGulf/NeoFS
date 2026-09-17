#ifndef PATH_H
#define PATH_H

#include <stdio.h>
#include <stdbool.h>

int get_path_meta_block(char *path, bool allow_creation, FILE *disk);

#endif