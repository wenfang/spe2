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

typedef struct {
  speTask_t*  readTask;
  speTask_t*  writeTask;
  unsigned    mask:2;             // mask set in epoll
} speEpoll_t __attribute__((aligned(sizeof(long))));

static int        				epfd;
static int        				epoll_maxfd;
static speEpoll_t 				*all_epoll;
static struct epoll_event *epEvents;

/*
===================================================================================================
epoll_change
===================================================================================================
*/
static bool
epoll_change(unsigned fd, speEpoll_t* epoll_t, unsigned newmask) {
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
SpeEpollEnable
===================================================================================================
*/
bool
SpeEpollEnable(unsigned fd, unsigned mask, speTask_t* task) {
  ASSERT(task);
  if (fd >= epoll_maxfd) return false;
  speEpoll_t* epoll_t = &all_epoll[fd];
  if (mask & SPE_EPOLL_READ) epoll_t->readTask = task;
  if (mask & SPE_EPOLL_WRITE) epoll_t->writeTask = task;
  return epoll_change(fd, epoll_t, epoll_t->mask | mask);
}

/*
===================================================================================================
SpeEpollDisable
===================================================================================================
*/
bool
SpeEpollDisable(unsigned fd, unsigned mask) {
  if (fd >= epoll_maxfd) return false;
  speEpoll_t* epoll_t = &all_epoll[fd];
  if (mask & SPE_EPOLL_READ) epoll_t->readTask = NULL;
  if (mask & SPE_EPOLL_WRITE) epoll_t->writeTask = NULL;
  return epoll_change(fd, epoll_t, epoll_t->mask & (~mask));
}

/*
===================================================================================================
SpeEpollProcess
===================================================================================================
*/
void
SpeEpollProcess(int timeout) {
  int events_n = epoll_wait(epfd, epEvents, epoll_maxfd, timeout);
  if (unlikely(events_n < 0)) {
    if (errno == EINTR) return;
    SPE_LOG_ERR("epoll_wait error: %s", strerror(errno));
    return;
  }
  if (events_n == 0) return;
  // check events
  struct epoll_event* e;
  speEpoll_t* epoll_t;
  for (int i=0; i<events_n; i++) {
    e = &epEvents[i];
    epoll_t = &all_epoll[e->data.fd];
    if ((e->events & EPOLLIN) && (epoll_t->mask & SPE_EPOLL_READ)) {
      spe_task_schedule(epoll_t->readTask);
    }
    if ((e->events & EPOLLOUT) && (epoll_t->mask & SPE_EPOLL_WRITE)) {
      spe_task_schedule(epoll_t->writeTask);
    }
  }
}

/*
===================================================================================================
epollInit
===================================================================================================
*/
static bool
epollInit(speCycle_t *cycle) {
  epfd = epoll_create(10240);
  if (epfd < 0) {
    SPE_LOG_ERR("epoll_create: %s", strerror(errno));
    return false;
  }
  epoll_maxfd = cycle->maxfd;
  all_epoll = calloc(epoll_maxfd, sizeof(speEpoll_t));
  if (!all_epoll) {
    close(epfd);
    return false;
  }
	epEvents = calloc(epoll_maxfd, sizeof(struct epoll_event));
	if (!epEvents) {
		free(all_epoll);
		close(epfd);
		return false;
	}
  return true;
}

/*
===================================================================================================
epollExit
===================================================================================================
*/
static bool
epollExit(speCycle_t *cycle) {
  close(epfd);
  free(all_epoll);
	free(epEvents);
  return true;
}

speModule_t speEpollModule = {
  "speEpoll",
  0,
  SPE_CORE_MODULE,
  NULL,
  epollInit,
  epollExit,
  NULL,
};
