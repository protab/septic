#include "fs.h"
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/stat.h>
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

