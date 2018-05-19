#include <getopt.h>
#include <stdbool.h>
#include "common.h"
#include "config.h"
#include "fs.h"
#include "log.h"
#include "meta.h"
#include "process.h"
#include "users.h"

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

	proc_start(bin_dir, "test", argv[optind]);

	return 0;
}
