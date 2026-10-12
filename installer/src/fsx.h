/* Work-file operations required by github.c. */
#pragma once
#include <stddef.h>
#include <sys/types.h>

int fs_write_all(int fd, const void *data, size_t size);
int fs_create_work_file(const char *path, mode_t mode);
int fs_remove_work_file(const char *path);
