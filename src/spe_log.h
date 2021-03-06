#ifndef __SPE_LOG_H 
#define __SPE_LOG_H

#include <syslog.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

#define SPE_LOG_EMERG(fmt, arg...)     spe_log_write(LOG_EMERG|LOG_LOCAL6, "[%s:%d][%d][%x][EMERG]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_ALERT(fmt, arg...)     spe_log_write(LOG_ALERT|LOG_LOCAL6, "[%s:%d][%d][%x][ALERT]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_CRIT(fmt, arg...)      spe_log_write(LOG_CRIT|LOG_LOCAL6, "[%s:%d][%d][%x][CRIT]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_ERR(fmt, arg...)       spe_log_write(LOG_ERR|LOG_LOCAL6, "[%s:%d][%d][%x][ERROR]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_WARNING(fmt, arg...)   spe_log_write(LOG_WARNING|LOG_LOCAL6, "[%s:%d][%d][%x][WARNING]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_NOTICE(fmt, arg...)    spe_log_write(LOG_NOTICE|LOG_LOCAL6, "[%s:%d][[%d]%x][NOTICE]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_INFO(fmt, arg...)      spe_log_write(LOG_INFO|LOG_LOCAL6, "[%s:%d][%d][%x][INFO]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define SPE_LOG_DEBUG(fmt, arg...)     spe_log_write(LOG_DEBUG|LOG_LOCAL6, "[%s:%d][%d][%x][DEBUG]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg) 

static inline void 
spe_log_init(const char* name) {
  openlog(name, LOG_CONS|LOG_PID, LOG_LOCAL6);
}

extern void 
spe_log_write(int priority, const char* message, ...);

static inline void 
spe_log_close() {
  closelog();
}

#endif
