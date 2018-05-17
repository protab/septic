#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"
#include "config.h"
#include "fs.h"
#include "log.h"

static void prepare(char *bin_dir, int user, char *command)
{
	pid_t res;

	res = fork();
	check(res);
	if (res > 0) {
		int status;

		waitpid(res, &status, 0);
		if (!WIFEXITED(status)) {
			log_err("Could not %s the sandbox (%d)", command, status);
			exit(1);
		}
		if (WEXITSTATUS(status)) {
			log_err("Could not %s the sandbox (exit code %d)", command, WEXITSTATUS(status));
			exit(1);
		}
		return;
	}
	char *bin_path = ssprintf("%s/isolate.bin", bin_dir);
	check(execl(bin_path, bin_path,
		    "--silent",
		    ssprintf("--box-id=%d", user),
		    ssprintf("--%s", command),
		    NULL));
}

static void start(char *bin_dir, int user)
{
	prepare(bin_dir, user, "cleanup");
	prepare(bin_dir, user, "init");

	char *bin_path = ssprintf("%s/isolate.bin", bin_dir);
	check(execl(bin_path, bin_path,
		    "--silent",
		    ssprintf("--box-id=%d", user),
		    "--wall-time=30",
		    "--mem=50000",
		    ssprintf("--meta=%s/run/%d", METAFS_DIR, user),
		    "--no-default-dirs",
		    "--dir=box=./box",
		    ssprintf("--dir=etc=%s/root/etc", bin_dir),
		    ssprintf("--dir=lib=%s/root/lib", bin_dir),
		    ssprintf("--dir=lib64=%s/root/lib64", bin_dir),
		    ssprintf("--dir=usr=%s/root/usr", bin_dir),
		    "--dir=proc=proc:fs",
		    "--env=HOME=/box",
		    "--run",
		    "/usr/bin/python",
		    NULL));
}

int main(int argc __unused, char **argv)
{
	char *bin_dir;

	log_init("<septic>", false);
	mkdir_meta();
	bin_dir = get_bin_dir(argv[0]);

	start(bin_dir, 0);

	return 0;
}
