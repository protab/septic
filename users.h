#ifndef USERS_H
#define USERS_H

#define LOGIN_LEN	30

void usr_init(void);
void usr_reload(void);
int usr_get_uid(const char *login);

#endif
