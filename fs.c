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

	tmp = realpath(argv0, NULL);
	if (!tmp)
		check(errno);
	res = sstrdup(dirname(tmp));
	free(tmp);
	return res;
}

void smkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res < 0 && errno != EEXIST)
		check(res);
}

void mkdir_meta(void)
{
	char *path;

	smkdir(METAFS_DIR, 0777);
	path = ssprintf("%s/run", METAFS_DIR);
	smkdir(path, 0777);
	sfree(path);
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
		char *end;
		long fd = strtol(e->d_name, &end, 10);

		if (*end)
			continue;
		if ((fd >= 0 && fd <= 2) || fd == dirfd(dir))
			continue;
		close(fd);
	}
	closedir(dir);
}
