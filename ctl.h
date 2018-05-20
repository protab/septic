#ifndef CTL_H
#define CTL_H
#include <stdbool.h>

enum ctl_action {
	CTL_NONE,
	CTL_RUN,
	CTL_KILL,
};

struct ctl_request {
	char *login;
	char *master;
	char *prg;
	int max_secs;
	int action;
};

void ctl_init(void);
bool ctl_parse(int fd, struct ctl_request *req);
void ctl_report(int fd, const char *msg);
void ctl_request_free(struct ctl_request *req);

void ctl_client_init(void);
void ctl_client_send(struct ctl_request *req);
char *ctl_client_get(void);

#endif
