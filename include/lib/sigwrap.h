#pragma once
#include <stdio.h>

// TODO fmemopen
// TODO fopencookie
// TODO sprintf
// TODO snprintf

// TODO mmap
// TODO munmap

void*  sw_malloc (size_t size);
void*  sw_calloc (size_t nmemb, size_t size);
void*  sw_realloc(void* ptr, size_t size);
size_t sw_fread  (void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t sw_fwrite (const void* ptr, size_t size, size_t nmemb, FILE* stream);
FILE*  sw_fopen  (const char* pathname, const char* mode);
int    sw_fclose (FILE* stream);
int    sw_fflush (FILE* stream);
int    sw_fsync  (int fd);
int    sw_msync  (void* addr, size_t len, int flags);
int    sw_fseek  (FILE* stream, long offset, int whence);
int    sw_printf (const char* format, ...);
int    sw_fprintf(FILE* stream, const char* format, ...);
void*  sw_mmap   (void* addr, size_t len, int prot, int flags, int fd, off_t off);
int    sw_munmap (void* addr, size_t len);

// fcntl is varargs but without a vfcntl designed for forwarding these nicely.
//
// Instead, we'll wrap it with a macro.
#define sw_fcntl(fd, cmd, ...) ({ \
  int _fcntl_ret = fcntl(fd, cmd, ##__VA_ARGS__); \
  if (_fcntl_ret == -1) { \
    switch (errno) { \
      case EPERM:  \
      case EACCES: \
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "fcntl(): Permission denied", NULL, NULL); \
        break; \
      case EBADF: \
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "fcntl(): Bad file descriptor", NULL, NULL); \
        break; \
      case EINTR: \
        SIG_SEND(SIGNAL_INTERRUPTED, "fcntl(): Interupted", NULL, NULL); \
        break; \
      case EINVAL: \
        SIG_SEND(SIGNAL_INVALID_INPUT, "fcntl(): Invalid input", NULL, NULL); \
        break;\
      case EMFILE: \
        SIG_SEND(SIGNAL_TOO_MANY_OPEN_FILES, "fcntl(): Too many open files", NULL, NULL); \
        break; \
      case ENOLCK: \
        SIG_SEND(SIGNAL_TOO_MANY_LOCKS, "fcntl(): Too many locks", NULL, NULL); \
        break; \
      case EOVERFLOW: \
        SIG_SEND(SIGNAL_OVERFLOW, "fcntl(): overflow", NULL, NULL); \
        break; \
      case ESRCH: \
        SIG_SEND(SIGNAL_NOT_FOUND, "fcntl(): process not found", NULL, NULL); \
        break; \
      case EDEADLK: \
        SIG_SEND(SIGNAL_WOULD_DEADLOCK, "fcntl(): would deadlock", NULL, NULL); \
        break; \
      default: \
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "fcntl(): unknown error", NULL, NULL); \
    } \
  } \
  _fcntl_ret; \
})
