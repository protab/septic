#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "ctl.h"
#include "log.h"

static void help(char *argv0)
{
	printf(
		"Usage: %s [OPTION]...\n"
		"\n"
		"Mandatory arguments:\n"
		"  -p, --prog=PROGRAM     program to run in sandbox\n"
		"  -m, --master=PROGRAM   master program\n"
		"\n"
		"Optional arguments:\n"
		"  -u, --user=LOGIN       user [test]\n"
		"  -t, --time=SECONDS     maximum time the program can run [30]\n"
		"  -h, --help             this help\n",
		argv0
	      );
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
		{ "help", no_argument, NULL, 'h' },
		{ 0 }
	};
	int opt;
	struct ctl_request req;

	log_init("client", false);

	memset(&req, 0, sizeof(req));
	req.login = "test";
	req.max_secs = 30;

	while ((opt = getopt_long(argc, argv, "p:m:u:t:h", longopts, NULL)) >= 0) {
		switch (opt) {
		case 'p':
			req.prg = to_path(optarg);
			break;
		case 'm':
			req.master = to_path(optarg);
			break;
		case 'u':
			req.login = optarg;
			break;
		case 't':
			to_int(optarg, &req.max_secs);
			break;
		case 'h':
			help(argv[0]);
			return 0;
		default:
			return 1;
		}
	}

	if (!req.prg || !req.master) {
		log_err("missing -p or -m argument (try -h for help)");
		return 1;
	}
	ctl_client_init();
	ctl_client_send(&req);
	return 0;
}
