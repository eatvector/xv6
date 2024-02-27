struct stat;
#include"kernel/thread.h"

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(const char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
void *mmap(void *addr, uint length, int prot, int flags,
                  int fd, uint offset);
int munmap(void *addr, uint length);


int thread_create(int *tid,void *attr,void *(start)(void*),void *args);
void thread_exit(void *retval);
int thread_join(int tid,void **retval);
int thread_self(void);
int thread_mutex_init(struct pthread_mutex_t *mutex,void *attr);
int  thread_mutex_lock(struct pthread_mutex_t *mutex);
int  thread_mutex_unlock(struct pthread_mutex_t *mutex);
int thread_cond_init(struct pthread_cond_t *cond,void *attr);
int  thread_cond_wait(struct pthread_cond_t *cond);
int  thread_cond_signal(struct pthread_cond_t *cond);
























// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
