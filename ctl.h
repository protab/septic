#ifndef CTL_H
#define CTL_H
#include <stdbool.h>

struct ctl_request {
	char *login;
	char *master;
	char *prg;
	int max_secs;
};

void ctl_init(void);
void ctl_accept(void);
bool ctl_parse(int fd, struct ctl_request *req);
void ctl_request_free(struct ctl_request *req);

void ctl_client_init(void);
void ctl_client_send(struct ctl_request *req);

#endif
