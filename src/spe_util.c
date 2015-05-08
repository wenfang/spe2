#include "spe_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/*
===================================================================================================
SpeDaemon
===================================================================================================
*/
int 
SpeDaemon() {
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
SpeSavePid
===================================================================================================
*/
bool
SpeSavePid(const char* pid_file) {
  ASSERT(pid_file);
  FILE *fp;
  if (!(fp = fopen(pid_file, "w"))) return false;
  fprintf(fp, "%ld\n", (long)getpid());
  if (fclose(fp) == -1) return false;
  return true;
}

/*
===================================================================================================
SpeGetPid
===================================================================================================
*/
pid_t
SpeGetPid(const char* pid_file) {
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
SpeRemovePid
===================================================================================================
*/
bool
SpeRemovePid(const char* pid_file) {
  ASSERT(pid_file);
  if (unlink(pid_file)) return false;
  return true;
}

extern char** environ;

static char** speArgv;
static char* speArgvLast = NULL;

/*
===================================================================================================
SpeInitProctitle
===================================================================================================
*/
bool
SpeInitProctitle(int argc, char** argv) {
	/*
	speArgv = calloc(1, (argc+1)*sizeof(char*));
	if (speArgv == NULL) return false;

	size_t len = 0;
	for (int i=0; i<argc; i++) {
		len = strlen(argv[i]) + 1;
		speArgv[i] = calloc(1, len);
		if (speArgv[i] == NULL) return false;
		strncpy(speArgv[i], argv[i], len);
	}
	speArgv[argc] = NULL;
	*/
	speArgv = argv;

	size_t size = 0;
	for (int i=0; environ[i]; i++) {
		size += strlen(environ[i]) + 1;
	}

	char* p = calloc(1, size);
	if (p == NULL) return false;

	speArgvLast =  speArgv[0];
	for (int i=0; speArgv[i]; i++) {
		if (speArgvLast == speArgv[i]) speArgvLast = speArgv[i] + strlen(speArgv[i]) + 1;
	}

	for (int i=0; environ[i]; i++) {
		if (speArgvLast == environ[i]) {
			size = strlen(environ[i]) + 1;
			speArgvLast = environ[i] + size;
			strncpy(p, environ[i], size);
			environ[i] = p;
			p += size;
		}
	}

	speArgvLast--;

	return true;
}

/*
===================================================================================================
SpeSetProcTitle
===================================================================================================
*/
void
SpeSetProctitle(char* title) {
	speArgv[1] = NULL;
	strncpy(speArgv[0], title, speArgvLast - speArgv[0]);
}
