#include "meta.h"
#include <dirent.h>
#include <sys/types.h>
#include "common.h"
#include "config.h"
#include "fs.h"

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
		long number;

		if (!to_long(e->d_name, &number))
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
