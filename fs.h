#ifndef FS_H
#define FS_H
#include <stdio.h>
#include <sys/types.h>

char *get_bin_dir(char *argv0);
void smkdir(const char *path, mode_t mode);
void sunlink(const char *path);
void ssymlink(const char *src, const char *dst);
int cp(const char *src, const char *dst);

/* Reads one line from 'stream' and returns up to 'size' characters
 * (including the terminating null). The newline character is stripped.
 * If the line was longer than the buffer, the remaining characters are
 * discarded. */
char *fgetline(char *s, int size, FILE *stream);

/* Close all file descriptors except for 0, 1 and 2. */
void close_fds(void);

#endif
