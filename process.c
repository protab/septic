#include "process.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"
#include "fs.h"
#include "log.h"
#include "meta.h"
#include "users.h"

static void prepare(const char *bin_path, int uid, char *command)
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
	check_sys(execl(bin_path, bin_path,
			"--silent",
			ssprintf("--box-id=%d", uid),
			ssprintf("--%s", command),
			NULL));
}

void proc_start(const char *bin_dir, const char *login, const char *prg)
{
	int uid = usr_get_uid(login);
	char *meta_dir, *bin_path;

	log_info("starting for user %s, program %s", login, prg);
	if (uid < 0) {
		log_err("uknown user %s", login);
		return;
	}

	if (meta_running(login)) {
		log_err("user %s: already running", login);
		return;
	}

	bin_path = ssprintf("%s/isolate.bin", bin_dir);
	prepare(bin_path, uid, "cleanup");
	prepare(bin_path, uid, "init");

	meta_dir = meta_new(login);
	log_info("meta directory %s", meta_dir);
	if (meta_cp_prg(prg, meta_dir, uid) < 0) {
		log_err("cannot copy %s", prg);
		sfree(bin_path);
		return;
	}

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
			"program.py",
			NULL));
}
