#include "libevsig/sigwrap.h"
#include "libevsig/errno_signals.h"
#include <errno.h>
#include <stdlib.h>
#include "__stdarg_va_arg.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "libevsig/signals.h"
#include "stdio.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <arpa/inet.h>

void* sw_malloc(size_t size) {
  void* out = malloc(size);

  if (!out) SIG_SEND(SIGNAL_ALLOC_FAILED, "Memory allocation failed", NULL, NULL);

  return out;
}

void* sw_realloc(void* ptr, size_t size) {
  void* out = realloc(ptr, size);

  if (!out) SIG_SEND(SIGNAL_ALLOC_FAILED, "Memory allocation failed", NULL, NULL);

  return out;
}

void* sw_calloc(size_t nmemb, size_t size) {
  void* out = calloc(nmemb, size);

  if (!out) SIG_SEND(SIGNAL_ALLOC_FAILED, "Memory allocation failed", NULL, NULL);

  return out;
}

size_t sw_fread(void* ptr, size_t size, size_t nmemb, FILE* stream) {
  // TODO implement/use sw_feof? sw_ferror below?
  if (!stream) {
    SIG_SEND(SIGNAL_INVALID_INPUT, "fread: Can't read from NULL stream", NULL, NULL);
    return -1;
  }
  if (feof(stream)) {
    SIG_SEND(SIGNAL_EOF, "fread: Unexpected end of file (EOF)", NULL, NULL);
    return 0;
  }

  size_t out = fread(ptr, size, nmemb, stream);

  // From fread docs:
  //
  // If an error occurs, or the end of the file is reached, the return value is
  // a short item count (or zero).
  //
  // fread()  does  not  distinguish  between  end-of-file  and  error, and
  // callers must use feof(3) and ferror(3) to determine which occurred.

  // fread() does not neccesarily set errno. There's a POSIX extension that asks for this,
  // but not all implementations use it.

  // explain_fread() exists and could give us a more detailed message, but it's not thread-safe.
  // Lack of thread safety makes this useless to us here.

  if (out != nmemb && ferror(stream)) {
    SIG_SEND(SIGNAL_READ_ERROR, "fread error. explain_fread() might have more information.",
             NULL, NULL);
  }

  // TODO I don't think this is true, consider when size = 1
  // out == 0 would mean that the user read up to EOF, but then tried to read again.
  //if (out == 0 && feof(stream)) {
  //  r1 = sig_send(SIGNAL_EOF, "End of file during fread(). "
  //                            "This occurs if you reached EOF then tried to read again.");
  //  sent_signal = true;
  //}

  return out;
}

size_t sw_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) {
  if (!stream) {
    SIG_SEND(SIGNAL_INVALID_INPUT, "fwrite: Can't write to NULL stream", NULL, NULL);
  }

  size_t out = fwrite(ptr, size, nmemb, stream);

  if (out != nmemb) {
    SIG_SEND(SIGNAL_WRITE_ERROR, "fwrite error", NULL, NULL);
  }

  return out;
}

// TODO grep for all fopen usage and replace
FILE* sw_fopen(const char* pathname, const char* mode) {
  FILE* out = fopen(pathname, mode);

  // fopen: If NULL is returned, we have an error. Will then set errno.

  if (!out) SIG_SEND(sig_from_errno(errno), str_from_errno("fopen(): ", errno),
                     NULL, NULL);

  return out;
}

int sw_fclose(FILE* stream) {
  int out = fclose(stream);

  if (out) SIG_SEND(sig_from_errno(errno), str_from_errno("fclose(): ", errno),
                    NULL, NULL);

  return out;
}

int sw_fflush(FILE* stream) {
  int out = fflush(stream);

  if (out) SIG_SEND(sig_from_errno(errno), str_from_errno("fflush(): ", errno),
                    NULL, NULL);

  return out;
}

int sw_munmap(void* addr, size_t len) {
  int out = munmap(addr, len);

  if (out) SIG_SEND(sig_from_errno(errno), str_from_errno("munmap(): ", errno),
                    NULL, NULL);

  return out;
}

void* sw_mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off) {
  void* out = mmap(addr, len, prot, flags, fd, off);

  if (out == MAP_FAILED) {
    SIG_SEND(sig_from_errno(errno), str_from_errno("mmap(): ", errno), NULL, NULL)
  }

  return out;
}

int sw_msync(void* addr, size_t len, int flags) {
  int out = msync(addr, len, flags);

  if (out) SIG_SEND(sig_from_errno(errno), str_from_errno("msync(): ", errno),
                    NULL, NULL);

  return out;
}

int sw_fsync(int fd) {
  int out = fsync(fd);

  if (out) SIG_SEND(sig_from_errno(errno), str_from_errno("fsync(): ", errno),
                    NULL, NULL);

  return out;
}

int sw_fseek(FILE* stream, long offset, int whence) {
  if (!stream) {
    SIG_SEND(SIGNAL_INVALID_INPUT, "fseek: Can't seek NULL stream", NULL, NULL);
  }

  int out = fseek(stream, offset, whence);


  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("fseek(): ", errno),
             NULL, NULL);

  return out;
}

int sw_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  int out = vprintf(format, args);
  va_end(args);

  // printf documentation doesn't specify any errno values. All we know is if it failed.

  if (out < 0) {
    SIG_SEND(SIGNAL_FAIL, "printf failed", NULL, NULL);
  }

  return out;
}

int sw_fprintf(FILE* stream, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int out = vfprintf(stream, format, args);
  va_end(args);

  // printf documentation doesn't specify any errno values. All we know is if it failed.

  if (out < 0) {
    SIG_SEND(SIGNAL_FAIL, "fprintf failed", NULL, NULL);
  }

  return out;
}

int sw_fcntl3(int fd, int cmd, uint64_t a) {
  int out = fcntl(fd, cmd, a);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("fcntl(): ", errno),
             NULL, NULL);

  return out;
}

int sw_fcntl2(int fd, int cmd) {
  int out = fcntl(fd, cmd);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("fcntl(): ", errno),
             NULL, NULL);

  return out;
}

const char* sw_inet_ntop(int af, const void *restrict src,
                         char dst[], socklen_t size) {
  const char* out = inet_ntop(af, src, dst, size);

  if (!out)
    SIG_SEND(sig_from_errno(errno), str_from_errno("net_ntop(): ", errno),
             NULL, NULL);

  return out;
}

ssize_t sw_read(int fd, void* buf, size_t nbyte) {
  ssize_t out = read(fd, buf, nbyte);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("read(): ", errno),
             NULL, NULL);

  return out;
}


ssize_t sw_write(int fd, void* buf, size_t nbyte) {
  ssize_t out = write(fd, buf, nbyte);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("write(): ", errno),
             NULL, NULL);

  return out;
}
