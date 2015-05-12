#include "spe_epoll.h"
#include "spe_sock.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <errno.h>

typedef struct spe_epoll_s {
  unsigned    mask:2;             // mask set in epoll
  spe_task_t* read_task;
  spe_task_t* write_task;
} spe_epoll_t __attribute__((aligned(sizeof(long))));

static int        				epfd;
static int        				epoll_maxfd;
static spe_epoll_t 				*epolls;
static struct epoll_event *epoll_events;

/*
===================================================================================================
change_epoll
===================================================================================================
*/
static bool
change_epoll(unsigned fd, spe_epoll_t* epoll_t, unsigned newmask) {
  if (epoll_t->mask == newmask) return true;
  // set epoll_event 
  struct epoll_event ee;
  ee.data.u64 = 0;
  ee.data.fd  = fd;
  ee.events   = 0;
  if (newmask & SPE_EPOLL_READ) ee.events |= EPOLLIN;
  if (newmask & SPE_EPOLL_WRITE) ee.events |= EPOLLOUT;
  // set op type 
  int op = EPOLL_CTL_MOD; 
  if (epoll_t->mask == SPE_EPOLL_NONE) {
    op = EPOLL_CTL_ADD;
  } else if (newmask == SPE_EPOLL_NONE) {
    op = EPOLL_CTL_DEL; 
  }
  if (epoll_ctl(epfd, op, fd, &ee) == -1) {
    SPE_LOG_ERR("epoll_ctl error: fd %d, %s", fd, strerror(errno));
    return false;
  }
  epoll_t->mask = newmask;
  return true;
}

/*
===================================================================================================
spe_epoll_enable
===================================================================================================
*/
bool
spe_epoll_enable(unsigned fd, unsigned mask, spe_task_t* task) {
  ASSERT(task);
  if (fd >= epoll_maxfd) return false;
  spe_epoll_t* epoll_t = &epolls[fd];
  if (mask & SPE_EPOLL_READ) epoll_t->read_task = task;
  if (mask & SPE_EPOLL_WRITE) epoll_t->write_task = task;
  return change_epoll(fd, epoll_t, epoll_t->mask | mask);
}

/*
===================================================================================================
spe_epoll_disable
===================================================================================================
*/
bool
spe_epoll_disable(unsigned fd, unsigned mask) {
  if (fd >= epoll_maxfd) return false;
  spe_epoll_t* epoll_t = &epolls[fd];
  if (mask & SPE_EPOLL_READ) epoll_t->read_task = NULL;
  if (mask & SPE_EPOLL_WRITE) epoll_t->write_task = NULL;
  return change_epoll(fd, epoll_t, epoll_t->mask & (~mask));
}

/*
===================================================================================================
spe_epoll_process
===================================================================================================
*/
void
spe_epoll_process(int timeout) {
  int events_n = epoll_wait(epfd, epoll_events, epoll_maxfd, timeout);
  if (unlikely(events_n < 0)) {
    if (errno == EINTR) return;
    SPE_LOG_ERR("epoll_wait error: %s", strerror(errno));
    return;
  }
  if (events_n == 0) return;
  // check events
  struct epoll_event* e;
  spe_epoll_t* epoll_t;
  for (int i=0; i<events_n; i++) {
    e = &epoll_events[i];
    epoll_t = &epolls[e->data.fd];
    if ((e->events & EPOLLIN) && (epoll_t->mask & SPE_EPOLL_READ)) {
      spe_task_schedule(epoll_t->read_task);
    }
    if ((e->events & EPOLLOUT) && (epoll_t->mask & SPE_EPOLL_WRITE)) {
      spe_task_schedule(epoll_t->write_task);
    }
  }
}

/*
===================================================================================================
init_epoll
===================================================================================================
*/
static bool
init_epoll(spe_cycle_t *cycle) {
  epfd = epoll_create(10240);
  if (epfd < 0) {
    SPE_LOG_ERR("epoll_create: %s", strerror(errno));
    return false;
  }

  epoll_maxfd = cycle->maxfd;
  epolls = calloc(epoll_maxfd, sizeof(spe_epoll_t));
  if (!epolls) {
    close(epfd);
    return false;
  }

	epoll_events = calloc(epoll_maxfd, sizeof(struct epoll_event));
	if (!epoll_events) {
		free(epolls);
		close(epfd);
		return false;
	}
  return true;
}

/*
===================================================================================================
exit_epoll
===================================================================================================
*/
static bool
exit_epoll(spe_cycle_t *cycle) {
  close(epfd);
  free(epolls);
	free(epoll_events);
  return true;
}

spe_module_t spe_epoll_module = {
  "spe_epoll",
  0,
  SPE_CORE_MODULE,
  NULL,
  init_epoll,
  exit_epoll,
  NULL,
};
