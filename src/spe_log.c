#include "spe_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#define MAX_MSG_LEN 512

/*
===================================================================================================
spe_log_init
===================================================================================================
*/
void 
spe_log_init(const char* name) {
  openlog(name, LOG_CONS|LOG_PID, LOG_LOCAL6);
}

/*
===================================================================================================
spe_log_write
===================================================================================================
*/
void 
spe_log_write(int priority, const char* message, ...) {
  char msg[MAX_MSG_LEN+1]; 
  va_list ap; 
    
  va_start(ap, message);
  vsnprintf(msg, MAX_MSG_LEN, message, ap);
  syslog(priority, "%s", msg);
  va_end(ap);
}

/*
===================================================================================================
spe_log_close
===================================================================================================
*/
void 
spe_log_close() {
  closelog();
}
