#include <stdio.h>
#include <string.h>
#include "../config.h"

#define BUF_SIZE	1024

int main(void)
{
	char buf[BUF_SIZE];
	char *pos;

	while (fgets(buf, BUF_SIZE, stdin)) {
		pos = strchr(buf, '\n');
		if (pos)
			*pos = 0;

		if (!strncmp("box_root", buf, 8)) {
			pos = strchr(buf, '=');
			if (pos && (size_t)(pos - buf) < BUF_SIZE - 1 - sizeof(HOMEFS_DIR)) {
				pos[1] = ' ';
				strcpy(pos + 2, HOMEFS_DIR);
			}
		}
		puts(buf);
	}
	return 0;
}
