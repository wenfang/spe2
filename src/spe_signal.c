#include "spe_signal.h"
#include <string.h>
#include <signal.h>

#define MAX_SIGNAL 256

typedef struct spe_signal_s {
  unsigned            cnt;
  spe_signal_handler* handler;
} spe_signal_t;

static int          signal_queue_len;
static int          signal_queue[MAX_SIGNAL];
static spe_signal_t signal_state[MAX_SIGNAL];
static sigset_t     blocked_signal;

/*
===================================================================================================
signal_init
===================================================================================================
*/
static bool
signal_init(spe_cycle_t *cycle) {
  signal_queue_len = 0;
  memset(signal_queue, 0, sizeof(signal_queue));
  memset(signal_state, 0, sizeof(signal_state));
  sigfillset(&blocked_signal);
  return true;
}

/*
===================================================================================================
signal_common_handler
===================================================================================================
*/
static void 
signal_common_handler(int sig) {
	// not handler this signal, ignore
  if (sig < 0 || sig > MAX_SIGNAL || !signal_state[sig].handler) {
    signal(sig, SIG_IGN);
    return;
  }
	// put signal in queue
  if (!signal_state[sig].cnt && (signal_queue_len<MAX_SIGNAL)) {
    signal_queue[signal_queue_len++] = sig;
  }
	// add signal cnt
  signal_state[sig].cnt++;
  signal(sig, signal_common_handler);
}

/*
===================================================================================================
spe_signal_register
===================================================================================================
*/
void 
spe_signal_register(int sig, spe_signal_handler handler) {
	if (sig < 0 || sig > MAX_SIGNAL) return;
	signal_state[sig].cnt = 0;
 	if (handler == NULL) handler = SIG_IGN; 
	// set signal handler
	if (handler != SIG_IGN && handler != SIG_DFL) {
		signal_state[sig].handler = handler;
 		signal(sig, signal_common_handler);
 	} else {                        
   	signal_state[sig].handler = NULL;
 		signal(sig, handler);
	}
}

/*
===================================================================================================
spe_signal_process
===================================================================================================
*/
void 
spe_signal_process(void) {
  if (signal_queue_len == 0) return;
	sigset_t old_sig;
	sigprocmask(SIG_SETMASK, &blocked_signal, &old_sig);
 	// check signal queue	
	for (int i=0; i<signal_queue_len; i++) {
		int sig = signal_queue[i];
 		spe_signal_t* desc = &signal_state[sig];
		if (desc->cnt) {
   		if (desc->handler) desc->handler(sig);
   		desc->cnt = 0;
 		}
 	}
	signal_queue_len = 0;
	sigprocmask(SIG_SETMASK, &old_sig, NULL);  
}

spe_module_t spe_signal_module = {
  "spe_signal",
  0,
  SPE_CORE_MODULE,
  signal_init,
  NULL,
  NULL,
  NULL,
};
