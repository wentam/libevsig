#include "lib/sigwrap.h"
#include <errno.h>
#include <stdlib.h>
#include "__stdarg_va_arg.h"
#include "asm-generic/errno-base.h"
#include "asm-generic/errno.h"
#include "lib/signals.h"
#include "stdio.h"
#include <sys/mman.h>
#include <unistd.h>

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

  if (!out) {
    switch (errno) {
      case EACCES:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "fopen: permission denied", NULL, NULL);
        break;
      case EEXIST:
        SIG_SEND(SIGNAL_FILE_EXISTS, "fopen: file already exists", NULL, NULL);
        break;
      case EIO:
        SIG_SEND(SIGNAL_IO_ERROR, "fopen: I/O error", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fopen: invalid input", NULL, NULL);
        break;
      case EISDIR:
        SIG_SEND(SIGNAL_IS_DIRECTORY, "fopen: is a directory", NULL, NULL);
        break;
      case EMFILE:
        SIG_SEND(SIGNAL_TOO_MANY_OPEN_FILES, "fopen: too many open files", NULL, NULL);
        break;
      case ENOENT:
        SIG_SEND(SIGNAL_NO_SUCH_FILE_OR_DIR, "fopen: no such file or directory", NULL, NULL);
        break;
      case ENOMEM:
        SIG_SEND(SIGNAL_NOT_ENOUGH_MEMORY, "fopen: not enough memory", NULL, NULL);
        break;
      case ENXIO:
        SIG_SEND(SIGNAL_NO_SUCH_DEVICE, "fopen: no such device or address", NULL, NULL);
        break;
      case EROFS:
        SIG_SEND(SIGNAL_READ_ONLY_FS, "fopen: read only filesystem", NULL, NULL);
        break;
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "fopen: bad file descriptor", NULL, NULL);
        break;
      case EBUSY:
        SIG_SEND(SIGNAL_BUSY, "fopen: busy / device in use", NULL, NULL);
        break;
      case EDQUOT:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fopen: not enough space", NULL, NULL);
        break;
      case EFAULT:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fopen: pathname outside address space", NULL, NULL);
        break;
      case EFBIG:
        SIG_SEND(SIGNAL_FILE_TOO_BIG, "fopen: file too big", NULL, NULL);
        break;
      case EINTR:
        SIG_SEND(SIGNAL_INTERRUPTED, "fopen: interrupted by signal", NULL, NULL);
        break;
      case ELOOP:
        SIG_SEND(SIGNAL_TOO_MANY_SYMLINKS, "fopen: too many symbolic links", NULL, NULL);
        break;
      case ENAMETOOLONG:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fopen: pathname too long", NULL, NULL);
        break;
      case ENFILE:
        SIG_SEND(SIGNAL_TOO_MANY_OPEN_FILES, "fopen: too many open files", NULL, NULL);
        break;
      case ENODEV:
        SIG_SEND(SIGNAL_NO_SUCH_DEVICE, "fopen: no such device", NULL, NULL);
        break;
      case ENOSPC:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fopen: not enough space", NULL, NULL);
        break;
      case ENOTDIR:
        SIG_SEND(SIGNAL_IS_NOT_DIRECTORY, "fopen: not a directory", NULL, NULL);
        break;
      case EOPNOTSUPP:
        SIG_SEND(SIGNAL_UNSUPPORTED_OP, "fopen: unsupported operation", NULL, NULL);
        break;
      case EOVERFLOW:
        SIG_SEND(SIGNAL_FILE_TOO_BIG, "fopen: file too big", NULL, NULL);
        break;
      case EPERM:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "fopen: permission denied", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "fopen: unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_fclose(FILE* stream) {
  int out = fclose(stream);

  if (out) {
    switch(errno) {
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "fclose: bad file descriptor", NULL, NULL);
        break;
      case EINTR:
        SIG_SEND(SIGNAL_INTERRUPTED, "fclose: interrupted by signal", NULL, NULL);
        break;
      case EIO:
        SIG_SEND(SIGNAL_IO_ERROR, "fclose: I/O error", NULL, NULL);
        break;
      case ENOSPC:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fclose: not enough space", NULL, NULL);
        break;
      case EDQUOT:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fclose: not enough space", NULL, NULL);
        break;
      case EAGAIN:
        SIG_SEND(SIGNAL_WOULD_BLOCK, "fclose: would block", NULL, NULL);
        break;
      case EFAULT:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fclose: buf outside address space", NULL, NULL);
        break;
      case EFBIG:
        SIG_SEND(SIGNAL_FILE_TOO_BIG, "fclose: file too big", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fclose: invalid input", NULL, NULL);
        break;
      case EPERM:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "fclose: permission denied", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "fclose: unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_fflush(FILE* stream) {
  int out = fflush(stream);

  if (out) {
    switch(errno) {
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "fflush: bad file descriptor", NULL, NULL);
        break;
      case EINTR:
        SIG_SEND(SIGNAL_INTERRUPTED, "fflush: interrupted by signal", NULL, NULL);
        break;
      case EIO:
        SIG_SEND(SIGNAL_IO_ERROR, "fflush: I/O error", NULL, NULL);
        break;
      case ENOSPC:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fflush: not enough space", NULL, NULL);
        break;
      case EDQUOT:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fflush: not enough space", NULL, NULL);
        break;
      case EAGAIN:
        SIG_SEND(SIGNAL_WOULD_BLOCK, "fflush: would block", NULL, NULL);
        break;
      case EFAULT:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fflush: buf outside address space", NULL, NULL);
        break;
      case EFBIG:
        SIG_SEND(SIGNAL_FILE_TOO_BIG, "fflush: file too big", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fflush: invalid input", NULL, NULL);
        break;
      case EPERM:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "fflush: permission denied", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "fflush: unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_munmap(void* addr, size_t len) {
  int out = munmap(addr, len);

  if (out) {
    switch(errno) {
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "munmap: invalid input", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "munmap: unknown error", NULL, NULL);
    }
  }

  return out;
}

void* sw_mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off) {
  void* out = mmap(addr, len, prot, flags, fd, off);

  if (out == MAP_FAILED) {
    switch (errno) {
      case EOVERFLOW:
        SIG_SEND(SIGNAL_FILE_TOO_BIG, "mmap: requested map region exceeds file size", NULL, NULL);
        break;
      case ENXIO:
        SIG_SEND(SIGNAL_NO_SUCH_DEVICE, "mmap: no such device or address", NULL, NULL);
        break;
      case ENOTSUP:
        SIG_SEND(SIGNAL_UNSUPPORTED_OP, "mmap: unsupported operation", NULL, NULL);
        break;
      case ENOMEM:
        SIG_SEND(SIGNAL_NOT_ENOUGH_MEMORY, "mmap: not enough memory", NULL, NULL);
        break;
      case ENODEV:
        SIG_SEND(SIGNAL_NO_SUCH_DEVICE, "mmap: no such device (file type not supported by mmap?)", NULL, NULL);
        break;
      case EMFILE:
        SIG_SEND(SIGNAL_TOO_MANY_OPEN_FILES, "mmap: too many mapped regions", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "mmap: invalid input", NULL, NULL);
        break;
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "mmap: bad file descriptor", NULL, NULL);
        break;
      case EAGAIN:
        SIG_SEND(SIGNAL_WOULD_BLOCK, "mmap: mapping could not be locked in memory", NULL, NULL);
        break;
      case EACCES:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "mmap: permission denied (file open for reading?)", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "mmap: unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_msync(void* addr, size_t len, int flags) {
  int out = msync(addr, len, flags);

  if (out) {
    switch (errno) {
      case ENOMEM:
        SIG_SEND(SIGNAL_NOT_ENOUGH_MEMORY, "msync: not enough memory", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "msync: invalid input", NULL, NULL);
        break;
      case EBUSY:
        SIG_SEND(SIGNAL_BUSY, "msync: busy / device in use", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "msync: unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_fsync(int fd) {
  int out = fsync(fd);

  if (out) {
    switch (errno) {
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "fsync: bad file descriptor", NULL, NULL);
        break;
      case EINTR:
        SIG_SEND(SIGNAL_INTERRUPTED, "fsync: interrupted by signal", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fsync: invalid input", NULL, NULL);
        break;
      case EIO:
        SIG_SEND(SIGNAL_IO_ERROR, "fsync: I/O error", NULL, NULL);
        break;
      case EAGAIN:
        SIG_SEND(SIGNAL_WOULD_BLOCK, "fsync: would block", NULL, NULL);
        break;
      case EISDIR:
        SIG_SEND(SIGNAL_IS_DIRECTORY, "fsync: is a directory", NULL, NULL);
        break;
      case EOVERFLOW:
        SIG_SEND(SIGNAL_FILE_TOO_BIG, "fsync: file too big", NULL, NULL);
        break;
      case EFBIG:
        SIG_SEND(SIGNAL_FILE_TOO_BIG, "fsync: file too big", NULL, NULL);
        break;
      case ENOSPC:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fsync: not enough space", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "fsync: unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_fseek(FILE* stream, long offset, int whence) {
  if (!stream) {
    SIG_SEND(SIGNAL_INVALID_INPUT, "fseek: Can't seek NULL stream", NULL, NULL);
  }

  int out = fseek(stream, offset, whence);

  if (out == -1) {
    switch (errno) {
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fseek: invalid argument(s)", NULL, NULL);
        break;
      case ESPIPE:
        SIG_SEND(SIGNAL_NOT_SEEKABLE, "fseek: stream is not seekable", NULL, NULL);
        break;
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "fseek: bad file descriptor", NULL, NULL);
        break;
      case EINTR:
        SIG_SEND(SIGNAL_INTERRUPTED, "fseek: interrupted by signal", NULL, NULL);
        break;
      case EIO:
        SIG_SEND(SIGNAL_IO_ERROR, "fseek: I/O error", NULL, NULL);
        break;
      case ENOSPC:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fseek: not enough space", NULL, NULL);
        break;
      case EDQUOT:
        SIG_SEND(SIGNAL_NOT_ENOUGH_SPACE, "fseek: not enough space", NULL, NULL);
        break;
      case EAGAIN:
        SIG_SEND(SIGNAL_WOULD_BLOCK, "fseek: would block", NULL, NULL);
        break;
      case EFAULT:
        SIG_SEND(SIGNAL_INVALID_INPUT, "fseek: buf outside address space", NULL, NULL);
        break;
      case EFBIG:
        SIG_SEND(SIGNAL_FILE_TOO_BIG, "fseek: file too big", NULL, NULL);
        break;
      case EPERM:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "fseek: permission denied", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "fseek: unknown error", NULL, NULL);
    }
  }

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

// TODO sprintf
// TODO snprintf


// TODO grep for all of these and replace with wrappers
