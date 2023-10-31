#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#define BUF_SIZE 512


int matchhere(char*, char*);
int matchstar(int, char*, char*);

/*int
match(char *re, char *text)
{
  if(re[0] == '^')
    return matchhere(re+1, text);
  do{  // must look at empty string
    if(matchhere(re, text))
      return 1;
  }while(*text++ != '\0');
  return 0;
}*/

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text)
{
  if(re[0] == '\0')
    return text[0]=='\0';
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text)
{
  do{  // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  }while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}


char *basename(const char *path) {
  static char buf[BUF_SIZE];
  strcpy(buf, path);
  char *p;
  for (p = buf + strlen(path); p >= buf && *p != '/'; p--)
    ;
  p++;
  static char name[BUF_SIZE];
  strcpy(name, p);

  /*for(int i=0;i<strlen(name);i++){
    if(name[i]=='\^'){
      name[i]='\^';
    }else if(name[i]=='\$'){
      name[i]='\$';
    }else if(name[i]=='\*'){
      name[i]='\*';
    }else if(name[i]=='\.'){
      name[i]='\.';
    }
  }*/

  return name;
}

void find( char *path,  char *pattern) {
  
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
      find(buf, pattern);
    }
  }else if(st.type==T_FILE){
      if(matchhere(pattern,basename(path))){
      printf("%s\n", path);
  }
  }
  close(fd);
}

int main(int argc, char *argv[]) {

 if(argc <3){
    fprintf(2, "usage: find  directory pattern\n");
    exit(1);
  }

  if(strcmp(argv[1],basename(argv[1]))!=0){
    fprintf(2,"not a basename\n");
    exit(1);
  }

  find(argv[1], argv[2]);
  exit(0);
}
