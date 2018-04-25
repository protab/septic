#include <libgen.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"
#include "config.h"
#include "errno.h"
#include "log.h"

static char *get_bin_path(char *argv0)
{
	char *tmp, *res;

	tmp = realpath(argv0, NULL);
	if (!tmp)
		check(errno);
	res = sstrdup(dirname(tmp));
	free(tmp);
	return res;
}

static void prepare(char *bin_path, char *command)
{
	pid_t res;

	res = fork();
	check(res);
	if (res > 0) {
		int status;

		waitpid(res, &status, 0);
		if (!WIFEXITED(status)) {
			log_err("Could not init the sandbox (%d)", status);
			exit(1);
		}
		if (WEXITSTATUS(status)) {
			log_err("Could not init the sandbox (exit code %d)", WEXITSTATUS(status));
			exit(1);
		}
		return;
	}
	char *bin = ssprintf("%s/isolate.bin", bin_path);
	check(execl(bin, bin,
		    "--silent",
		    ssprintf("--config=%s/isolate.conf", bin_path),
		    "--box-id=0",
		    ssprintf("--%s", command),
		    NULL));
}

static void start(char *bin_path)
{
	prepare(bin_path, "cleanup");
	prepare(bin_path, "init");

	char *bin = ssprintf("%s/isolate.bin", bin_path);
	check(execl(bin, bin,
		    "--silent",
		    ssprintf("--config=%s/isolate.conf", bin_path),
		    "--box-id=0",
		    "--wall-time=30",
		    "--mem=50000",
//		    ssprintf("--meta=%s/meta", HOMEFS_DIR),
		    "--no-default-dirs",
		    "--dir=box=./box",
		    ssprintf("--dir=etc=%s/root/etc", bin_path),
		    ssprintf("--dir=lib=%s/root/lib", bin_path),
		    ssprintf("--dir=lib64=%s/root/lib64", bin_path),
		    ssprintf("--dir=usr=%s/root/usr", bin_path),
		    "--dir=proc=proc:fs",
		    "--env=HOME=/box",
		    "--run",
		    "/usr/bin/python",
		    NULL));
}

int main(int argc __unused, char **argv)
{
	char *bin_path;

	log_init("<septic>", false);
	bin_path = get_bin_path(argv[0]);

	start(bin_path);

	return 0;
}
