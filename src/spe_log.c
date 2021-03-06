#include "spe_log.h"
#include <stdio.h>
#include <stdarg.h>

#define MAX_MSG_LEN 512

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
