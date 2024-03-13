#include "kernel/types.h"
#include "user/user.h"

// pipe2.c: communication between two processes

int
main()
{
  int n, pid;
  int fds[2];
  char buf[100];
  
  // create a pipe, with two FDs in fds[0], fds[1].
  pipe(fds);

  pid = fork();
  // if we are kid we use fd[1] to write some data
  if (pid == 0) {
    write(fds[1], "this is pipe2\n", 14);
  } else {
  // if we are parents we just read  data 
    n = read(fds[0], buf, sizeof(buf));
    write(1, buf, n);
  }

  exit(0);
}