#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#define BUF_SIZE 512


char *basename(const char *path) {
  static char buf[BUF_SIZE];
  strcpy(buf, path);
  char *p;
  for (p = buf + strlen(path); p >= buf && *p != '/'; p--)
    ;
  p++;
  static char name[BUF_SIZE];
  strcpy(name, p);
  return name;
}

void find(const char *path, const char *filename) {
  // printf("%s %s\n",path,filename);
  char buf[BUF_SIZE], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (strcmp(basename(path), filename) == 0) {
    printf("%s\n", path);
  }
  if (st.type == T_DIR) {
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      printf("ls: path too long\n");
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0 || strcmp(de.name, ".") == 0 ||
          strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      //printf("buf %s\n", buf);
      find(buf, filename);
    }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(2, "Error:please find path filename\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}
