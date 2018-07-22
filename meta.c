#include "meta.h"
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "common.h"
#include "config.h"
#include "fs.h"

void meta_init(void)
{
	smkdir(META_DIR, 0777);
}

static bool parse_line(char *buf, char **key, char **val)
{
	char *saveptr;

	*key = strtok_r(buf, ":", &saveptr);
	*val = strtok_r(NULL, ":", &saveptr);
	return *key && *val;
}

int meta_get_status(const char *login, long id, struct meta_status_info *info)
{
	char *path;
	FILE *f;
	char buf[256];

	memset(info, 0, sizeof(*info));
	path = ssprintf("%s/%s/%ld/status", META_DIR, login, id);
	f = fopen(path, "r");
	sfree(path);
	if (!f)
		return -errno;
	while (fgetline(buf, sizeof(buf), f)) {
		char *key, *val;

		info->finished = true;
		if (!parse_line(buf, &key, &val))
			continue;
		if (!strcmp(key, "killed"))
			info->killed = true;
		else if (!strcmp(key, "exitcode"))
			to_int(val, &info->exitcode);
		else if (!strcmp(key, "message"))
			strlcpy(info->message, val, sizeof(info->message));
		else if (!strcmp(key, "status")) {
			if (strcmp(val, "RE"))
				info->killed = true;
		}

	}
	fclose(f);

	path = ssprintf("%s/%s/%ld/task_pid", META_DIR, login, id);
	f = fopen(path, "r");
	sfree(path);
	if (f) {
		fgets(buf, sizeof(buf), f);
		to_int(buf, &info->task_pid);
		fclose(f);
	}

	return 0;
}

bool meta_running(const char *login)
{
	struct meta_status_info info;

	return !meta_get_status(login, 0, &info) && !info.finished;
}

char *meta_new(const char *login)
{
	char *base, *path, *dst, *src;
	DIR *dir;
	struct dirent *e;
	long max = 0;

	base = ssprintf("%s/%s", META_DIR, login);
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
	check_sys(chmod(path, 0777));

	dst = ssprintf("%s/0", base);
	src = ssprintf("%ld", max);
	ssymlink(src, dst);

	sfree(src);
	sfree(dst);
	sfree(base);
	return path;
}

int meta_cp_prg(const char *src, const char *meta_dir, int uid)
{
	struct stat info;
	char *dst;
	int res;

	res = stat(src, &info);
	if (res < 0)
		return -errno;
	if (info.st_size > MAX_PRG_SIZE)
		return -E2BIG;

	dst = ssprintf("%s/program.py", meta_dir);
	res = cp(src, dst);
	sfree(dst);
	if (res < 0)
		return res;

	dst = ssprintf("%s/%d/box/program.py", HOME_DIR, uid);
	res = cp(src, dst);
	sfree(dst);
	if (res < 0)
		return res;

	return 0;
}

void meta_record_pid(const char *meta_dir, pid_t pid)
{
	char *dst;
	FILE *f;

	dst = ssprintf("%s/task_pid", meta_dir);
	check_ptr(f = fopen(dst, "w"));
	check_sys(fprintf(f, "%d", pid));
	fclose(f);
	sfree(dst);
}

void meta_record_token(const char *meta_dir, const char *token)
{
	char *dst;
	FILE *f;

	dst = ssprintf("%s/token", meta_dir);
	check_ptr(f = fopen(dst, "w"));
	check_sys(fprintf(f, "%s", token));
	fclose(f);
	sfree(dst);
}
