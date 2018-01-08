#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "play.h"

static int play_pid = 0;
static void (*play_finished_handler)(void) = NULL;
static char *play_program = NULL;

static void play_finished(int signo)
{
  int status;

  waitpid(play_pid, &status, 0);
  play_pid = 0;

  if (play_finished_handler != NULL)
    (play_finished_handler)();
}

void play_finished_handler_install(void (*handler)(void))
{
  play_finished_handler = handler;
}

void play_set_program(char *program)
{
  play_program = program;
}

void play_cancel(void)
{
  int status;
  struct sigaction sa;

  if (play_pid == 0)
    return; /* Nothing is executing, cannot cancel. */

  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGCHLD, &sa, NULL);

  kill(play_pid, SIGINT);

  waitpid(play_pid, &status, 0);
  play_pid = 0;
}

void play_execute(char *arg)
{
  struct sigaction sa;
  int fd0, fd1, fd2;

  if (play_program == NULL)
    return;

  if (play_pid > 0)
    return; /* Already running, cannot execute. */

  sa.sa_handler = play_finished;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGCHLD, &sa, NULL);

  play_pid = fork();
  if (play_pid == 0) {
    /* Redirect standard I/O to prevent child program writing to the TTY. */
    close(0);
    close(1);
    close(2);
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    execlp(play_program, play_program, arg, NULL);
  }
}

void play_block(void)
{
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, SIGCHLD);
  sigprocmask(SIG_BLOCK, &ss, NULL);
}

void play_unblock(void)
{
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, SIGCHLD);
  sigprocmask(SIG_UNBLOCK, &ss, NULL);
}

