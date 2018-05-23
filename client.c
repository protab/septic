#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"
#include "ctl.h"
#include "log.h"

static void help(char *argv0)
{
	printf(
		"Usage: %s [OPTION]...\n"
		"\n"
		"Running a program:\n"
		"  -p, --prog=PROGRAM     program to run in sandbox\n"
		"  -m, --master=PROGRAM   master program\n"
		"  -u, --user=LOGIN       [test] user\n"
		"  -t, --time=SECONDS     [30] maximum time the program can run\n"
		"\n"
		"Managing a running program:\n"
		"  -k, --kill             kill a running program\n"
		"  -w, --watch            watch the running program input/output\n"
		"\n"
		"  -h, --help             this help\n",
		argv0
	      );
}

static bool output(char *buf, ssize_t size, char prefix, bool newline, bool after)
{
	char sprefix[3] = { prefix, ' ', 0 };
	char *start, *ptr;

	for (start = buf; start - buf < size; start = ptr + 1) {
		if (newline)
			fputs(sprefix, stdout);
		for (ptr = start; *ptr != '\0' && *ptr != '\n'; ptr++)
			;
		newline = (*ptr == '\n');
		*ptr = '\0';
		fputs(buf, stdout);
		if (newline)
			fputs("\n", stdout);
	}
	if (newline && after) {
		fputs(sprefix, stdout);
		newline = false;
	}
	fflush(stdout);
	return newline;
}

static bool nonempty(char *fn)
{
	struct stat info;
	int res;

	res = stat(fn, &info);
	if (res < 0 && errno == ENOENT)
		return false;
	check_sys(res);
	return !!info.st_size;
}

#define BUF_SIZE	(128 * 1024 + 1)
static void watch(const char *meta_dir)
{
	char *buf = salloc(BUF_SIZE);
	char *fn_stat = ssprintf("%s/status", meta_dir);
	char *fn_output = ssprintf("%s/output", meta_dir);
	char *fn_input = ssprintf("%s/input", meta_dir);
	char *fn_tmp = ssprintf("%s/input.rrr", meta_dir);
	char *fn_ret = ssprintf("%s/input.ret", meta_dir);
	int fd_output;
	bool newline = true;
	bool seen = false;

	usleep(100000);
	while (1) {
		fd_output = open(fn_output, O_RDONLY | O_NONBLOCK);
		if (fd_output >= 0)
			break;
		if (errno != ENOENT)
			check_sys(fd_output);
		if (!seen)
			printf("* waiting\n");
		seen = true;
		usleep(500000);
	}
	printf("* connected\n");

	while (1) {
		ssize_t size;
		int fd;

		while (1) {
			check_sys(size = read(fd_output, buf, BUF_SIZE - 1));
			if (!size)
				break;
			buf[size] = '\0';
			newline = output(buf, size, '>', newline, false);
		}

		if (nonempty(fn_stat))
			break;

		fd = open(fn_input, O_RDONLY);
		if (fd < 0 && errno == ENOENT)
			goto again;
		check_sys(fd);
		check_sys(size = read(fd, buf, BUF_SIZE - 1));
		buf[size] = '\0';
		close(fd);
		if (!newline)
			fputs("\n", stdout);
		output(buf, size, '?', true, true);
		newline = true;

		fgets(buf, BUF_SIZE, stdin);
		size = strlen(buf);
		if (size && buf[size - 1] == '\n')
			buf[--size] = '\0';

		check_sys(fd = creat(fn_tmp, 0666));
		check_sys(write(fd, buf, size));
		close(fd);
		check_sys(rename(fn_tmp, fn_ret));
again:
		usleep(500000);
	}

	close(fd_output);
	if (!newline)
		fputs("\n", stdout);
	printf("* finished\n");
}

static char *to_path(const char *fname)
{
	char *res;

	check_ptr(res = realpath(fname, NULL));
	return res;
}

int main(int argc, char **argv)
{
	static const struct option longopts[] = {
		{ "prog", required_argument, NULL, 'p' },
		{ "master", required_argument, NULL, 'm' },
		{ "user", required_argument, NULL, 'u' },
		{ "time", required_argument, NULL, 't' },
		{ "kill", no_argument, NULL, 'k' },
		{ "help", no_argument, NULL, 'h' },
		{ 0 }
	};
	int opt;
	struct ctl_request req;
	char *meta_dir;

	log_init("client", false);

	memset(&req, 0, sizeof(req));
	req.login = "test";
	req.max_secs = 30;
	req.action = CTL_RUN;

	while ((opt = getopt_long(argc, argv, "p:m:u:t:kh", longopts, NULL)) >= 0) {
		switch (opt) {
		case 'p':
			req.prg = to_path(optarg);
			break;
		case 'm':
			req.master = optarg;
			break;
		case 'u':
			req.login = optarg;
			break;
		case 't':
			to_int(optarg, &req.max_secs);
			break;
		case 'k':
			req.action = CTL_KILL;
			break;
		case 'h':
			help(argv[0]);
			return 0;
		default:
			return 1;
		}
	}

	if (req.action == CTL_RUN && (!req.prg || !req.master)) {
		log_err("missing -p or -m argument (try -h for help)");
		return 1;
	}
	ctl_client_init();
	ctl_client_send(&req);
	printf("* %s\n", ctl_client_get(&meta_dir));
	if (meta_dir)
		watch(meta_dir);
	return 0;
}
