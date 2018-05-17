#ifndef FS_H
#define FS_H
#include <sys/types.h>

char *get_bin_dir(char *argv0);
void smkdir(const char *path, mode_t mode);
void mkdir_meta(void);

#endif
