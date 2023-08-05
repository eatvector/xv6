#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"
#define BUF_SIZE 1024

int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(2, "Error:too few arguments\n");
    exit(1);
  }
  char buf[BUF_SIZE];
  char *kid_argv[MAXARG];
  int i = 0;
 
  for (int j = 1; j < argc; j++) {
    kid_argv[i++] = argv[j];
  }
  int ai = i;
  

  for (;;) {
    gets(buf, BUF_SIZE);
    if (buf[0] == '\0') {
      break;
    }
    int len = strlen(buf);
    if (len >= 1 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
      buf[len - 1] = '\0';
    }
    i = ai;
    int space = 1;
    for (char *c = buf; *c; c++) {
      if (*c == ' ') {
        *c = '\0';
        space = 1;
      } else {
        if (space) {
          kid_argv[i++] = c;
          space = 0;
        }
      }
    }

    int kid_pid = fork();
    if (kid_pid > 0) {
      wait((int *)0);
    } else if (kid_pid == 0) {
      exec(argv[1], kid_argv);
      fprintf(2, "Error:exec %s fail\n", argv[1]);
      exit(1);
    } else {
      fprintf(2, "Error:fork fail\n");
      exit(1);
    }
  }
  exit(0);
}