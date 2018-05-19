#include <getopt.h>
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

static void start(const char *bin_dir, const char *login, const char *prg)
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

	log_info("meta directory %s", meta_dir);
	log_info("program %s", prg);
	if (meta_cp_prg(prg, meta_dir, uid) < 0) {
		log_err("cannot copy %s", prg);
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

static void help(char *argv0)
{
	printf(
		"Usage: %s [OPTION]...\n"
		"\n"
		"  -s, --syslog         log to syslog instead of stderr\n"
		"  -h, --help           this help\n",
		argv0
	      );
}

int main(int argc, char **argv)
{
	static const struct option longopts[] = {
		{ "syslog", no_argument, NULL, 's' },
		{ "help", no_argument, NULL, 'h' },
	};
	int opt;
	bool opt_syslog = false;
	char *bin_dir;

	while ((opt = getopt_long(argc, argv, "sh", longopts, NULL)) >= 0) {
		switch (opt) {
		case 's':
			opt_syslog = true;
			break;
		case 'h':
			help(argv[0]);
			return 0;
		default:
			return 1;
		}
	}

	if (optind >= argc) {
		help(argv[0]);
		return 1;
	}

	log_init("<septic>", opt_syslog);
	usr_init();
	meta_init();

	bin_dir = get_bin_dir(argv[0]);

	start(bin_dir, "test", argv[optind]);

	return 0;
}
