#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"
#include "config.h"
#include "fs.h"
#include "log.h"
#include "meta.h"
#include "users.h"

static void prepare(const char *bin_dir, int uid, char *command)
{
	pid_t res;

	check_sys(res = fork());
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
	check_sys(execl(bin_path, bin_path,
			"--silent",
			ssprintf("--box-id=%d", uid),
			ssprintf("--%s", command),
			NULL));
}

static void start(const char *bin_dir, const char *login)
{
	int uid = usr_get_uid(login);

	if (uid < 0) {
		log_err("Uknown user %s", login);
		return;
	}

	if (meta_running(login)) {
		log_err("already running");
		return;
	}

	prepare(bin_dir, uid, "cleanup");
	prepare(bin_dir, uid, "init");

	char *bin_path = ssprintf("%s/isolate.bin", bin_dir);
	char *meta_dir = meta_new(login);

	close_fds();
	check_sys(execl(bin_path, bin_path,
			"--silent",
			ssprintf("--box-id=%d", uid),
			"--wall-time=30",
			"--mem=50000",
			ssprintf("--meta=%s/isolate", meta_dir),
			"--inherit-fds",
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
	usr_init();
	meta_init();

	bin_dir = get_bin_dir(argv[0]);

	start(bin_dir, "test");

	return 0;
}
