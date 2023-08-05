#include "kernel/types.h"
#include "user/user.h"

int main() {
  int fds_w_kid[2];
  int fds_w_parent[2];
  pipe(fds_w_kid);
  pipe(fds_w_parent);
  int kid_pid=fork();
  if (kid_pid == -1) {
    fprintf(2, "Error:parent fork fail\n");
    exit(1);
  } else if (kid_pid > 0) {
    if (write(fds_w_kid[1], "a", 1) != 1) {
      fprintf(2, "Error:parent write pipe fail\n");
      exit(1);
    }
    char byte_read;
    if (read(fds_w_parent[0], &byte_read, 1) != 1) {
      fprintf(2, "Error:parent read pipe fail\n");
      exit(1);
    }
    int pid;
    if ((pid = getpid()) == -1) {
      fprintf(2, "Error:parent getpid fail\n");
      exit(1);
    }
    printf("%d: received pong\n",pid);
    exit(0);
  } else {
    char byte_read;
    if (read(fds_w_kid[0], &byte_read, 1) != 1) {
      fprintf(2, "Error:kid read pipe\n");
      exit(1);
    }
    int pid;
    if ((pid = getpid()) == -1) {
      fprintf(2, "Erroe:kid getpid fail\n");
      exit(1);
    }
    printf("%d: received ping\n",pid);
    if (write(fds_w_parent[1], &byte_read, 1) != 1) {
      fprintf(2, "Error:kid write pipe fail\n");
      exit(1);
    }
    exit(0);
  }
}