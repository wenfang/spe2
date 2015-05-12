#include "spe_util.h"
#include <fcntl.h>

/*
===================================================================================================
spe_daemon
===================================================================================================
*/
int 
spe_daemon() {
  switch (fork()) {
  case -1:
    return -1;
  case 0:   // child return here
    break;
  default:  // parent return and exit
    exit(EXIT_SUCCESS);
  }

  if (setsid() == -1) return -1;
  if (chdir("/") != 0) return -1;

  int fd;
  if ((fd = open("/dev/null", O_RDWR, 0)) == -1) return -1;
  if (dup2(fd, STDIN_FILENO) < 0) return -1;
  if (dup2(fd, STDOUT_FILENO) < 0) return -1;
  if (dup2(fd, STDERR_FILENO) < 0) return -1;
  if (fd > STDERR_FILENO) close(fd);
  return 0;
}

/*
===================================================================================================
spe_save_pid
===================================================================================================
*/
bool
spe_save_pid(const char* pid_file) {
  ASSERT(pid_file);
  FILE *fp;
  if (!(fp = fopen(pid_file, "w"))) return false;
  fprintf(fp, "%ld\n", (long)getpid());
  if (fclose(fp) == -1) return false;
  return true;
}

/*
===================================================================================================
spe_get_pid
===================================================================================================
*/
pid_t
spe_get_pid(const char* pid_file) {
  ASSERT(pid_file);
  long pid;
  FILE *fp;
  if (!(fp = fopen(pid_file, "r"))) return 0;
  fscanf(fp, "%ld\n", &pid);
  fclose(fp);
  return pid;
}

/*
===================================================================================================
spe_remove_pid
===================================================================================================
*/
bool
spe_remove_pid(const char* pid_file) {
  ASSERT(pid_file);
  if (unlink(pid_file)) return false;
  return true;
}

extern char** environ;

static char** spe_argv;
static char* spe_argv_last = NULL;

/*
===================================================================================================
spe_init_proc_title
===================================================================================================
*/
bool
spe_init_proc_title(int argc, char** argv) {
	spe_argv = argv;

	size_t size = 0;
	for (int i=0; environ[i]; i++) {
		size += strlen(environ[i]) + 1;
	}

	char* p = calloc(1, size);
	if (p == NULL) return false;

	spe_argv_last = spe_argv[0];
	for (int i=0; spe_argv[i]; i++) {
		if (spe_argv_last == spe_argv[i]) spe_argv_last = spe_argv[i] + strlen(spe_argv[i]) + 1;
	}

	for (int i=0; environ[i]; i++) {
		if (spe_argv_last == environ[i]) {
			size = strlen(environ[i]) + 1;
			spe_argv_last = environ[i] + size;
			strncpy(p, environ[i], size);
			environ[i] = p;
			p += size;
		}
	}

	spe_argv_last--;

	return true;
}

/*
===================================================================================================
spe_set_proc_title
===================================================================================================
*/
void
spe_set_proc_title(char* title) {
	spe_argv[1] = NULL;
	strncpy(spe_argv[0], title, spe_argv_last - spe_argv[0]);
}
