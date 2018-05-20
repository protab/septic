#include "event.h"
#include <signal.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common.h"
#include "log.h"
#include "users.h"

struct fd_data {
	int fd;
	event_callback_t cb;
	struct fd_data *next;
};

static int epfd;
static int sigfd;
static bool quit;
static struct fd_data *fd_head;

static void sig_handler(int fd, unsigned events __unused)
{
	struct signalfd_siginfo ssi;

	while (read(fd, &ssi, sizeof(ssi)) > 0) {
		switch (ssi.ssi_signo) {
		case SIGINT:
		case SIGTERM:
			log_info("terminating");
			event_quit();
			break;
		case SIGHUP:
			usr_reload();
			break;
		case SIGCHLD:
			while (true) {
				pid_t pid = waitpid(-1, NULL, WNOHANG);
				if (pid <= 0)
					break;
			}
			break;
		}
	}
}

void event_init(void)
{
	sigset_t sigs;

	check_sys(epfd = epoll_create1(EPOLL_CLOEXEC));
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGCHLD);
	sigaddset(&sigs, SIGINT);
	sigaddset(&sigs, SIGTERM);
	sigaddset(&sigs, SIGHUP);
	sigaddset(&sigs, SIGUSR1);
	sigaddset(&sigs, SIGUSR2);
	check_sys(sigprocmask(SIG_SETMASK, &sigs, NULL));
	check_sys(sigfd = signalfd(-1, &sigs, SFD_NONBLOCK | SFD_CLOEXEC));
	event_add(sigfd, EV_READ, sig_handler);
}

void event_cleanup(void)
{
	struct fd_data *ptr;

	close(sigfd);
	close(epfd);
	while (fd_head) {
		ptr = fd_head->next;
		sfree(fd_head);
		fd_head = ptr;
	}
}

void event_add(int fd, unsigned events, event_callback_t cb)
{
	struct fd_data *fdd;
	struct epoll_event e;

	fdd = salloc(sizeof(*fdd));
	fdd->fd = fd;
	fdd->cb = cb;
	fdd->next = fd_head;
	fd_head = fdd;

	e.events = events;
	e.data.ptr = fdd;
	check_sys(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &e));
}

#define EVENTS_MAX	64

void event_loop(void)
{
	struct epoll_event evbuf[EVENTS_MAX];
	int cnt;

	quit = false;
	while (!quit) {
		cnt = epoll_wait(epfd, evbuf, EVENTS_MAX, -1);
		if (cnt < 0 && errno == EINTR)
			continue;
		check_sys(cnt);
		for (int i = 0; i < cnt; i++) {
			struct fd_data *fdd = evbuf[i].data.ptr;

			fdd->cb(fdd->fd, evbuf[i].events);
		}
	}
}

void event_quit(void)
{
	quit = true;
}
