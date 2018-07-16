#ifndef META_H
#define META_H
#include <stdbool.h>
#include <sys/types.h>

#define MAX_PRG_SIZE	65536

struct meta_status_info {
	bool finished;
	bool killed;
	int exitcode;
	pid_t task_pid;
	char message[128];
};

void meta_init(void);
int meta_get_status(const char *login, long id, struct meta_status_info *info);
bool meta_running(const char *login);
char *meta_new(const char *login);
int meta_cp_prg(const char *src, const char *meta_dir, int uid);
void meta_record_pid(const char *meta_dir, pid_t pid);
void meta_record_token(const char *meta_dir, const char *token);

#endif
