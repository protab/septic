#ifndef META_H
#define META_H
#include <stdbool.h>

struct meta_status_info {
	bool finished;
	bool killed;
	int exitcode;
	char message[128];
};

void meta_init(void);
bool meta_running(const char *login);
char *meta_new(const char *login);

#endif
