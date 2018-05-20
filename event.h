#ifndef EVENT_H
#define EVENT_H
#include <sys/epoll.h>

#define EV_READ		EPOLLIN
#define EV_WRITE	EPOLLOUT
#define EV_ERROR	(EPOLERR | EPOLLHUP | EPOLLRDHUP)

typedef void (*event_callback_t)(int fd, unsigned events);

void event_init(void);
void event_cleanup(void);
void event_add(int fd, unsigned events, event_callback_t cb);
void event_loop(void);
void event_quit(void);

#endif
