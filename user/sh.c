// Shell.

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10
#define MAXFILENAME 64


#define MAXHISTORY 3
#define MAXCMDLEN  64

struct historycmds{
  char cmds[MAXHISTORY][MAXCMDLEN];
  int num;
  int pos;
};

struct historycmds historycmds;

void addcmdtohistorry(char *cmd){
  if(strlen(cmd)+1>MAXCMDLEN){
     return;
  }
  if(historycmds.num<MAXHISTORY){
    historycmds.num++;
  }
  strcpy(historycmds.cmds[historycmds.pos],cmd);
  historycmds.pos=(historycmds.pos+1)%MAXHISTORY;
}

void showhistory(){
  int num=historycmds.num;
  int i=historycmds.pos;
  for(int j=0;j<num;j++){
    i=(i+MAXHISTORY-1)%MAXHISTORY;
    printf("%s\n",historycmds.cmds[i]);
  }
}


int filecommand;
char commandfile[MAXFILENAME];


struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);
void runcmd(struct cmd*) __attribute__((noreturn));

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(1);

  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    //argv[0] is the program name;
    if(ecmd->argv[0] == 0)
      exit(1);
    // std 0 and 1 may point to pipe
    exec(ecmd->argv[0], ecmd->argv);
    fprintf(2, "exec %s failed\n", ecmd->argv[0]);
    break;
 
  //i do not understand what fuck is this
  case REDIR:
    rcmd = (struct redircmd*)cmd;
    // make rcmd->fd point to rcmd->file with mode rcmd->mode
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      fprintf(2, "open %s failed\n", rcmd->file);
      exit(1);
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    // let kid run the left cmd
    if(fork1() == 0)
      runcmd(lcmd->left);
    // wait kid to finish run left cmd
    wait(0);
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      // kid process 1 get write(pid 1)
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      // write to pid 1
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      //get data from pid 0
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    // wait for its two kid process to exit
    wait(0);
    wait(0);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    // kid process can still run,wen father process is exit
    // we just  folk a child to continue run cmd,and just exit,so the shell do not need to wait
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit(0);
}

void  stdintoconsole(){
      close(0);
      if(open("console", O_RDWR)<0){
        fprintf(2,"shell stdin redir to console fail\n");
        exit(1);
    }
}


void stdintofile(char *filename){
    close(0);
    if(open(filename,O_RDONLY)!=0){
        fprintf(2,"shell stdin redir to %s fail\n",filename);
        exit(1);
    }
}


int
getcmd(char *buf, int nbuf)
{
  if(!filecommand){
    write(2, "$ ", 2);
  }
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  //EOF
  if(buf[0] == 0){
    return -1;
  }
  return 0;
}


int
main(void)
{
  static char buf[100];
  int fd;

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  // 0 may point to a file
  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    //read file command end,stdin redir to console

    if(buf[0]==0){
      filecommand=0;
      commandfile[0]=0;
      stdintoconsole();
      continue;
    }else{
      // chop \n
      buf[strlen(buf)-1] = 0; 
    }
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Chdir must be called by the parent, not the child.
     // buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0){
        fprintf(2, "cannot cd %s\n", buf+3);
      }
        addcmdtohistorry(buf);
      continue;
    }
    // read command from file
    else if(buf[0]=='.'&&buf[1]==' '){
      // fork a new shell to process this
      addcmdtohistorry(buf);
      if(fork1()==0){
        strcpy(commandfile,buf+2);
        stdintofile(commandfile);
        filecommand=1; 
        continue;
      }
      wait(0);
      continue;
      //next time we will read command from file
    }else if(strcmp(buf,"history")==0){
      addcmdtohistorry(buf);
      showhistory();
      continue;
    }
   
    addcmdtohistorry(buf);

    // if we are child just runcmd
    // folk a child to run the cmd
    if(fork1() == 0){
      // stdin  may point to file ,we make it point to consloe
      if(filecommand){
        // i am not sure abou this
        stdintoconsole();
      }
      runcmd(parsecmd(buf));
    }
    wait(0);
  }
  exit(0);
}

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

//  a string from *ps to es
//  make *q point to the first nonwhitespace char in string [*ps,es) or es
//  *eq point to the world we want to process next,may be a white space(?)
//  *ps is after *eq, a nonwhitespace char or at es
//  ret may be symbols char or '+'(>>) or 'a' or 0
int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  // if is whitespace,just move on
  // strchr to find *s is in set whitespace,
  while(s < es && strchr(whitespace, *s))
    s++;
  // s now point to a nonwhitespace char
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++; 
    break;
  case '>':
    s++;
    // >> ,add something at the end of a file
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    // try to move s point to the next whitespace char or symbols char
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }

  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}


//  make *ps point to a nonwhitespace char or es
//  if the first nonwhitespace is in token return true,else return false
int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  // s now point to a nonwhitespace char
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  // check s point to a token
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){ 
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  // echo "hello world" >1.txt>2.txt will redir to 2.txt not 1,txt
  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE|O_TRUNC, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
      // not use ps,i do not understand what is eq
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}
// NUL-terminate all the counted strings.
// do some clean work
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}
