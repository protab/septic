#include "process.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"
#include "config.h"
#include "ctl.h"
#include "fs.h"
#include "log.h"
#include "meta.h"
#include "users.h"

static char *bin_dir;

void proc_init(char *argv0)
{
	bin_dir = get_bin_dir(argv0);
}

static void proc_kill(const char *login)
{
	struct meta_status_info info;

	if (meta_get_status(login, 0, &info) < 0 || info.finished) {
		log_info("nothing to kill");
		return;
	}
	log_info("killing task pid %d", info.task_pid);
	kill(info.task_pid, SIGKILL);
}

static void prepare_box(const char *bin_path, int uid, char *command)
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

	log_reinit("prepare");
	fd_to_null(1);
	fd_to_null(2);
	check_sys(execl(bin_path, bin_path,
			"--silent",
			ssprintf("--box-id=%d", uid),
			ssprintf("--%s", command),
			NULL));
}

static void reassign_pipe(int pin, int pout)
{
	/* The following is safe: we have 4 pipe descriptors, the first one
	 * in each pair is for reading. Worst case, dup2 will be a noop for
	 * some of the fds but we won't overwrite a fd that we still need to
	 * reassign. */
	check_sys(dup2(pin, 3));
	check_sys(dup2(pout, 4));
	close_fds(4);
}

static pid_t run_box(const char *bin_path, const char *meta_dir, int uid,
		     int max_secs, int pin, int pout)
{
	pid_t res;

	check_sys(res = fork());
	if (res > 0)
		return res;

	log_reinit("box");
	fd_to_null(0);
	reassign_pipe(pin, pout);

	check_sys(execl(bin_path, bin_path,
			"--silent",
			ssprintf("--box-id=%d", uid),
			ssprintf("--wall-time=%d", max_secs),
			"--mem=50000",
			ssprintf("--meta=%s/status", meta_dir),
			"--inherit-fds",
			"--no-default-dirs",
			"--dir=box=./box",
			ssprintf("--dir=etc=%s/root/etc", bin_dir),
			ssprintf("--dir=lib=%s/root/lib", bin_dir),
			ssprintf("--dir=lib64=%s/root/lib64", bin_dir),
			ssprintf("--dir=usr=%s/root/usr", bin_dir),
			"--dir=proc=proc:fs",
			"--env=HOME=/box",
			"--env=LANG=C.UTF-8",
			"--run",
			"/usr/bin/python",
			"/usr/bin/wrapper.py",
			NULL));
	return 0; /* can't happen but needed to shut up gcc */
}

static pid_t run_task(const char *meta_dir, const char *task, int pin, int pout)
{
	pid_t res;

	check_sys(res = fork());
	if (res > 0) {
		meta_record_pid(meta_dir, res);
		return res;
	}

	log_reinit("task");
	reassign_pipe(pin, pout);
	check_sys(execlp("python3", "python3",
			ssprintf("%s/python/master.py", INSTALL_DIR),
			TASKS_DIR,
			meta_dir,
			task,
			NULL));
	return 0; /* can't happen but needed to shut up gcc */
}

void proc_start(int fd)
{
	pid_t res, pid_task, pid_box;
	struct ctl_request req;
	char *meta_dir, *bin_path = NULL;
	int pfd[4];
	int uid;
	int err;
	int exitcode = 1;

	check_sys(res = fork());
	if (res > 0)
		return;

	log_reinit("control");
	log_info("getting parameters");
	if (!ctl_parse(fd, &req))
		exit(exitcode);
	log_info("action %d, user %s, task %s, prg %s, max_secs %d, token %s",
		 req.action, req.login, req.task, req.prg, req.max_secs, req.token);

	uid = usr_get_uid(req.login);
	if (uid < 0) {
		log_err("unknown user %s", req.login);
		ctl_report(fd, "unknown user", NULL);
		goto out_free;
	}

	if (req.action == CTL_KILL) {
		proc_kill(req.login);
		ctl_report(fd, "ok", NULL);
		goto out_free;
	}

	if (meta_running(req.login)) {
		log_err("user %s: already running", req.login);
		ctl_report(fd, "already running", NULL);
		goto out_free;
	}

	bin_path = ssprintf("%s/isolate.bin", bin_dir);
	prepare_box(bin_path, uid, "cleanup");
	prepare_box(bin_path, uid, "init");

	meta_dir = meta_new(req.login);
	log_info("meta directory %s", meta_dir);
	err = meta_cp_prg(req.prg, meta_dir, uid);
	if (err == -E2BIG) {
		log_err("file %s is over %d bytes", req.prg, MAX_PRG_SIZE);
		ctl_report(fd, "too large", NULL);
		goto out_free;
	} else if (err < 0) {
		log_err("cannot copy %s", req.prg);
		ctl_report(fd, "not found", NULL);
		goto out_free;
	}

	if (req.token)
		meta_record_token(meta_dir, req.token);

	ctl_report(fd, "ok", meta_dir);

	check_sys(pipe(pfd));
	check_sys(pipe(pfd + 2));

	pid_task = run_task(meta_dir, req.task, pfd[0], pfd[3]);
	log_info("task pid %d started", pid_task);

	pid_box = run_box(bin_path, meta_dir, uid, req.max_secs, pfd[2], pfd[1]);
	log_info("box pid %d started", pid_box);

	while (1) {
		res = wait(NULL);

		if (res == pid_task) {
			log_info("task pid %d finished, killing box pid %d", pid_task, pid_box);
			kill(pid_box, SIGTERM);
			break;
		} else if (res == pid_box) {
			log_info("box pid %d finished, killing task pid %d", pid_box, pid_task);
			kill(pid_task, SIGKILL);
			break;
		}
	}

	exitcode = 0;

out_free:
	sfree(bin_path);
	ctl_request_free(&req);
	exit(exitcode);
}
