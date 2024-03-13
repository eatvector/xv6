#include "kernel/types.h"
#include "user/user.h"

int first_prime = 1;

void primes() {
  int fds[2]; // connect to right

  int fd0; // connect to left
  int fd1;

  if (first_prime) {
    pipe(fds);
    for (int i = 2; i <= 35; i++) {
      write(fds[1], &i, sizeof(int));
    }
    first_prime = 0;
  }

  int n;
  int p;
  int get_p;
  int is_folk;
  int pid;
label:
  get_p = 0;
  is_folk = 0;
  fd0 = fds[0];
  fd1 = fds[1];

  if (close(fd1) == -1) {
    fprintf(2, "Error:close fd1 fail\n");
    exit(1);
  }

  while (read(fd0, &n, sizeof(int)) != 0) {
    if (get_p == 0) {
      p = n;
      get_p = 1;
      printf("prime %d\n", p);
    } else {
      if (n % p) {
        // fork child
        if (is_folk != 1) {
          is_folk = 1;
          pipe(fds); // pipe connect to its child
          pid = fork();
          if (pid == 0) {
            goto label;
          } else if (pid < 0) {
            fprintf(2, "Error:fork fail\n");
            exit(1);
          } else {
            if (close(fds[0]) == -1) {
              fprintf(2, "Error:close fds[0] fail\n");
              exit(1);
            }
          }
        }

        if (write(fds[1], &n, sizeof(int)) != sizeof(int)) {
          fprintf(2, "Error:fork fail\n");
          exit(1);
        }
      }
    }
  }
  
  if (close(fd0) == -1) {
    fprintf(2, "Error :close fd0 fail\n");
    exit(1);
  }
  if (is_folk) {
     if (close(fds[1]) == -1) {
      fprintf(2, "Error:close fds[1] fail\n");
    }
    wait((int *)0);
  }
  exit(0);
}

int main() {
  primes();
  return 0;
}