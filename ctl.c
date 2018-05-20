#include "ctl.h"
#include <errno.h>
#include <linux/un.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"
#include "config.h"
#include "log.h"
#include "fs.h"
#include "process.h"

static int usock = -1;
#define LISTEN_BACKLOG	20
#define RCV_BUF_SIZE	65536
#define MAX_KEY_SIZE	128
#define MAX_VAL_SIZE	65536

static void fill_sun(struct sockaddr_un *sun)
{
	memset(sun, 0, sizeof(*sun));
	sun->sun_family = AF_UNIX;
	snprintf(sun->sun_path, UNIX_PATH_MAX, "%s/socket", RUNFS_DIR);
}

void ctl_client_init(void)
{
	struct sockaddr_un sun;

	check_sys(usock = socket(AF_UNIX, SOCK_STREAM, 0));
	fill_sun(&sun);
	check_sys(connect(usock, (const struct sockaddr *)&sun, sizeof(sun)));
}

void ctl_init(void)
{
	struct sockaddr_un sun;

	smkdir(RUNFS_DIR, 0777);
	check_sys(usock = socket(AF_UNIX, SOCK_STREAM, 0));
	fill_sun(&sun);
	sunlink(sun.sun_path);
	check_sys(bind(usock, (const struct sockaddr *)&sun, sizeof(sun)));
	check_sys(listen(usock, LISTEN_BACKLOG));
}

void ctl_accept(void)
{
	int fd;

	while (1) {
		fd = accept(usock, NULL, NULL);
		if (fd < 0 && errno == EINTR)
			continue;
		check_sys(fd);
		break;
	}
	proc_start(fd);
	close(fd);
}

static bool store_str(char *key, char *val, char *match_key, char **dst)
{
	if (strcmp(key, match_key))
		return false;
	if (*dst) {
		log_err("duplicate %s field", match_key);
		sfree(*dst);
	}
	*dst = sstrdup(val);
	return true;
}

static bool store_int(char *key, char *val, char *match_key, int *dst)
{
	if (strcmp(key, match_key))
		return false;
	if (*dst)
		log_err("duplicate %s field", match_key);
	if (!to_int(val, dst))
		log_err("invalid %s field value", match_key);
	return true;
}

static void parse_one(char *key, char *val, struct ctl_request *req)
{
	store_str(key, val, "login", &req->login) ||
	store_str(key, val, "master", &req->master) ||
	store_str(key, val, "prg", &req->prg) ||
	store_int(key, val, "max_secs", &req->max_secs);
}

bool ctl_parse(int fd, struct ctl_request *req)
{
	char *buf = salloc(RCV_BUF_SIZE);
	char *key = salloc(MAX_KEY_SIZE);
	char *val = salloc(MAX_VAL_SIZE);
	char *bpos, *kpos, *vpos;
	bool in_key;
	ssize_t size;

	memset(req, 0, sizeof(*req));
	kpos = key;
	vpos = val;
	in_key = true;
	while (1) {
		check_sys(size = read(fd, buf, RCV_BUF_SIZE));
		if (!size)
			break;
		for (bpos = buf; bpos < buf + size; bpos++) {
			if (in_key) {
				if (*bpos == ':') {
					in_key = false;
					continue;
				}
				if (kpos < key + MAX_KEY_SIZE - 1)
					*kpos++ = *bpos;
			} else {
				if (*bpos == '\n') {
					*kpos = '\0';
					*vpos = '\0';
					parse_one(key, val, req);
					kpos = key;
					vpos = val;
					in_key = true;
					continue;
				}
				if (vpos < val + MAX_VAL_SIZE - 1)
					*vpos++ = *bpos;
			}
		}
	}
	if (!req->login || !req->master || !req->prg || !req->max_secs) {
		log_err("missing one of mandatory fields");
		return false;
	}
	return true;
}

void ctl_report(int fd, const char *msg)
{
	check_sys(write(fd, msg, strlen(msg)));
	close(fd);
}

void ctl_request_free(struct ctl_request *req)
{
	sfree(req->login);
	sfree(req->master);
	sfree(req->prg);
	memset(req, 0, sizeof(*req));
}

void ctl_client_send(struct ctl_request *req)
{
	char *data;

	data = ssprintf("login:%s\nmaster:%s\nprg:%s\nmax_secs:%d\n",
			req->login, req->master, req->prg, req->max_secs);
	check_sys(write(usock, data, strlen(data)));
	check_sys(shutdown(usock, SHUT_WR));
}

char *ctl_client_get(void)
{
	char *buf = salloc(RCV_BUF_SIZE);
	ssize_t size;

	check_sys(size = read(usock, buf, RCV_BUF_SIZE - 1));
	buf[size] = '\0';
	return buf;
}
