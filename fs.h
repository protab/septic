#ifndef FS_H
#define FS_H
#include <sys/types.h>

char *get_bin_dir(char *argv0);
void smkdir(const char *path, mode_t mode);
void sunlink(const char *path);
void ssymlink(const char *oldpath, const char *newpath);

/* Close all file descriptors except for 0, 1 and 2. */
void close_fds(void);

#endif
