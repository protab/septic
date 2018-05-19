#include "fs.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"

char *get_bin_dir(char *argv0)
{
	char *tmp, *res;

	check_ptr(tmp = realpath(argv0, NULL));
	res = sstrdup(dirname(tmp));
	free(tmp);
	return res;
}

void smkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res < 0 && errno != EEXIST)
		check_sys(res);
}

void sunlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res < 0 && errno != ENOENT)
		check_sys(res);
}

void ssymlink(const char *src, const char *dst)
{
	int res;

	res = symlink(src, dst);
	if (res < 0 && errno == EEXIST) {
		sunlink(dst);
		res = symlink(src, dst);
	}
	check_sys(res);
}

#define CP_BUF_SIZE	65536
int cp(const char *src, const char *dst)
{
	void *buf;
	int fd_src, fd_dst;
	ssize_t res_src = -1, res_dst;

	buf = salloc(CP_BUF_SIZE);
	fd_src = open(src, O_RDONLY);
	if (fd_src < 0)
		return -errno;
	check_sys(fd_dst = creat(dst, 0666));
	while (res_src) {
		check_sys(res_src = read(fd_src, buf, CP_BUF_SIZE));
		while (res_src > 0) {
			check_sys(res_dst = write(fd_dst, buf, res_src));
			res_src -= res_dst;
		}
	}
	close(fd_dst);
	close(fd_src);
	return 0;
}

static bool strip_nl(char *s)
{
	int len = strlen(s);

	if (len && s[len - 1] == '\n') {
		s[len - 1] = '\0';
		return true;
	}
	return false;
}

char *fgetline(char *s, int size, FILE *stream)
{
	char *res;
	bool consume;

	res = fgets(s, size, stream);
	if (!res)
		return res;
	consume = !strip_nl(s);
	while (consume) {
		char buf[64];

		if (!fgets(buf, sizeof(buf), stream))
			break;
		consume = !strip_nl(buf);
	}
	return res;
}

void close_fds(void)
{
	/* Based on the code in isolate/util.c. Note that we may have
	 * a fd to syslog opened at this point but it seems to be safe to
	 * close it: the vsyslog call will reopen it. */
	DIR *dir;
	struct dirent *e;

	check_ptr(dir = opendir("/proc/self/fd"));
	while ((e = readdir(dir))) {
		long fd;

		if (!to_long(e->d_name, &fd))
			continue;
		if ((fd >= 0 && fd <= 2) || fd == dirfd(dir))
			continue;
		close(fd);
	}
	closedir(dir);
}
