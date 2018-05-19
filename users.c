#include "users.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "config.h"
#include "fs.h"
#include "log.h"

struct user {
	char login[LOGIN_LEN + 1];
	int uid;
	struct user *next;
};

static struct user *users = NULL;

static void free_users(void)
{
	struct user *next;

	while (users) {
		next = users->next;
		sfree(users);
		users = next;
	}
}

static void add_user(const char *login, int uid)
{
	struct user *u;

	u = salloc(sizeof(*u));
	strlcpy(u->login, login, LOGIN_LEN + 1);
	u->uid = uid;
	u->next = users;
	users = u;
}

int usr_get_uid(const char *login)
{
	struct user *u;

	for (u = users; u; u = u->next)
		if (!strcmp(login, u->login))
			return u->uid;
	return -1;
}

static int parse_line(char *buf, char **login)
{
	char *saveptr, *suid;
	int uid;

	suid = strtok_r(buf, " \t", &saveptr);
	*login = strtok_r(NULL, " \t", &saveptr);
	if (!suid || !*login)
		return -1;
	if (!to_int(suid, &uid) || uid < 0)
		return -2;
	return uid;
}

/* When 'stream' is not NULL, works like fgetline. If 'stream' is NULL, returns
 * a default line, "0 test" and then EOF. To reset EOF condition, call with
 * null 'stream' and negative 'size'. */
static char *fgetline_default(char *s, int size, FILE *stream)
{
	static bool done;

	if (!stream) {
		if (size < 0) {
			done = false;
			return NULL;
		}
		if (done)
			return NULL;
		strlcpy(s, "0 test", size);
		done = true;
		return s;
	}
	return fgetline(s, size, stream);
}

void usr_reload(void)
{
	FILE *f;
	char *fname = DB_PATH;
	int line = 0, cnt = 0;
	char buf[LOGIN_LEN + 128];

	log_info("loading users from %s", DB_PATH);
	f = fopen(DB_PATH, "r");
	if (!f) {
		log_err("cannot open file %s, using a test user", DB_PATH);
		fgetline_default(NULL, -1, NULL);
		fname = "<test_user>";
	}

	free_users();
	while (fgetline_default(buf, sizeof(buf), f)) {
		int uid;
		char *login;

		line++;
		uid = parse_line(buf, &login);
		if (uid == -1)
			log_warn("%s:%d: invalid format", fname, line);
		else if (uid == -2)
			log_warn("%s:%d: invalid uid", fname, line);
		else {
			add_user(login, uid);
			cnt++;
		}
	}
	log_info("loaded %d users", cnt);
}

void usr_init(void)
{
	usr_reload();
}
