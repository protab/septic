#include "fs.h"
#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"
#include "config.h"

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

static void sunlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res < 0 && errno != ENOENT)
		check_sys(res);
}

static void ssymlink(const char *oldpath, const char *newpath)
{
	int res;

	res = symlink(oldpath, newpath);
	if (res < 0 && errno == EEXIST) {
		sunlink(newpath);
		res = symlink(oldpath, newpath);
	}
	check_sys(res);
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

void meta_mkdir(void)
{
	smkdir(METAFS_DIR, 0777);
}

char *meta_new(const char *login)
{
	char *base, *path, *dst, *src;
	DIR *dir;
	struct dirent *e;
	long max = 0;

	base = ssprintf("%s/%s", METAFS_DIR, login);
	dir = opendir(base);
	if (!dir && errno == ENOENT) {
		smkdir(base, 0777);
		dir = opendir(base);
	}
	check_ptr(dir);
	while ((e = readdir(dir))) {
		char *end;
		long number = strtol(e->d_name, &end, 10);

		if (*end)
			continue;
		if (number > max)
			max = number;
	}
	closedir(dir);

	max++;
	path = ssprintf("%s/%ld", base, max);
	smkdir(path, 0777);

	dst = ssprintf("%s/last", base);
	src = ssprintf("%ld", max);
	ssymlink(src, dst);

	sfree(src);
	sfree(dst);
	sfree(base);
	return path;
}
