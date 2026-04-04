#include "lib/signals.h"
#include <sys/socket.h>
#include <errno.h>

int sw_socket(int domain, int type, int protocol) {
  int out = socket(domain, type, protocol);

  if (out == -1) {
    switch (errno) {
      case EAFNOSUPPORT:
        SIG_SEND(SIGNAL_UNSUPPORTED, "socket(): Address family not supported", NULL, NULL);
        break;
      case EMFILE:
        SIG_SEND(SIGNAL_TOO_MANY_OPEN_FILES, "socket(): Too many open files", NULL, NULL);
        break;
      case ENFILE:
        SIG_SEND(SIGNAL_TOO_MANY_OPEN_FILES, "socket(): Too many open files", NULL, NULL);
        break;
      case EPROTONOSUPPORT:
        SIG_SEND(SIGNAL_UNSUPPORTED, "socket(): Unsupported protocol", NULL, NULL);
        break;
      case EPROTOTYPE:
        SIG_SEND(SIGNAL_UNSUPPORTED, "socket(): Socket type not supported by protocol", NULL, NULL);
        break;
      case EACCES:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "socket(): Permission denied", NULL, NULL);
        break;
      case ENOBUFS:
        SIG_SEND(SIGNAL_INSUFFICIENT_RESOURCES, "socket(): Insufficient resources", NULL, NULL);
        break;
      case ENOMEM:
        SIG_SEND(SIGNAL_NOT_ENOUGH_MEMORY, "socket(): Insufficient memory available", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "socket(): unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_setsockopt(int socket,
                  int level,
                  int option_name,
                  const void* option_value,
                  socklen_t option_len) {
  int out = setsockopt(socket, level, option_name, option_value, option_len);

  if (out == -1) {
    switch (errno) {
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "setsockopt: Bad file descriptor", NULL, NULL);
        break;
      case EDOM:
        SIG_SEND(SIGNAL_UNSUPPORTED, "setsockopt: Timeout values too large for socket structure", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "setsockopt: Invalid option", NULL, NULL);
        break;
      case EISCONN:
        SIG_SEND(SIGNAL_SOCK_ALREADY_CONNECTED,
                 "setsockopt: Socket already connected, but this option can't be set while connected",
                 NULL, NULL);
        break;
      case ENOPROTOOPT:
        SIG_SEND(SIGNAL_UNSUPPORTED, "setsockopt: Option not supported", NULL, NULL);
        break;
      case ENOTSOCK:
        SIG_SEND(SIGNAL_NOT_SOCKET, "setsockopt: Socket argument does not refer to socket", NULL, NULL);
        break;
      case ENOMEM:
        SIG_SEND(SIGNAL_NOT_ENOUGH_MEMORY, "setsockopt: Not enough memory", NULL, NULL);
        break;
      case ENOBUFS:
        SIG_SEND(SIGNAL_INSUFFICIENT_RESOURCES, "setsockopt: Insufficient resources", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "setsockopt: setsockopt: unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_bind(int socket, const struct sockaddr* address, socklen_t address_len) {
  int out = bind(socket, address, address_len);

  if (out == -1) {
    switch(out) {
      case EADDRINUSE:
        SIG_SEND(SIGNAL_ADDRESS_IN_USE, "bind: Address already in use", NULL, NULL);
        break;
      case EADDRNOTAVAIL:
        SIG_SEND(SIGNAL_ADDRESS_NOT_AVAILABLE, "bind: Address not available from local machine",
                 NULL, NULL);
        break;
      case EAFNOSUPPORT:
        SIG_SEND(SIGNAL_UNSUPPORTED, "bind: Specified address not valid/supported", NULL, NULL);
        break;
      case EALREADY:
        SIG_SEND(SIGNAL_BUSY, "bind: An assignment request is already in progress for this socket",
                 NULL, NULL);
        break;
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "bind: Bad file descriptor", NULL, NULL);
        break;
      case EINPROGRESS:
        SIG_SEND(SIGNAL_IN_PROGRESS,
                 "bind: O_NONBLOCK is set, but we can't bind immediately. Doing it asychronously.",
                 NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_UNSUPPORTED, "bind: Invalid option, socket already bound, or socket shutdown",
                 NULL, NULL);
        break;
      case ENOBUFS:
        SIG_SEND(SIGNAL_INSUFFICIENT_RESOURCES, "bind: Insufficient resources", NULL, NULL);
        break;
      case ENOTSOCK:
        SIG_SEND(SIGNAL_NOT_SOCKET, "bind: Socket argument does not refer to socket", NULL, NULL);
        break;
      case EOPNOTSUPP:
        SIG_SEND(SIGNAL_UNSUPPORTED, "bind: Socket type does not support binding", NULL, NULL);
        break;
      case EACCES:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "bind: Permission denied", NULL, NULL);
        break;
      case EDESTADDRREQ:
        SIG_SEND(SIGNAL_INVALID_INPUT, "bind: Address is NULL", NULL, NULL);
        break;
      case EISDIR:
        SIG_SEND(SIGNAL_INVALID_INPUT, "bind: Address is NULL", NULL, NULL);
        break;
      case EIO:
        SIG_SEND(SIGNAL_IO_ERROR, "bind: I/O error", NULL, NULL);
        break;
      case ELOOP:
        SIG_SEND(SIGNAL_LOOP, "bind: Infinite loop (of symlinks?) encountered resolving address",
                 NULL, NULL);
        break;
      case ENAMETOOLONG:
        SIG_SEND(SIGNAL_NAME_TOO_LONG, "bind: Name too long", NULL, NULL);
        break;
      case ENOENT:
        SIG_SEND(SIGNAL_INVALID_INPUT, "bind: Invalid path", NULL, NULL);
        break;
      case ENOTDIR:
        SIG_SEND(SIGNAL_INVALID_INPUT, "bind: Invalid path", NULL, NULL);
        break;
      case EROFS:
        SIG_SEND(SIGNAL_READ_ONLY_FS, "bind: Read-only filesystem", NULL, NULL);
        break;
      case EISCONN:
        SIG_SEND(SIGNAL_SOCK_ALREADY_CONNECTED,
                 "bind: Socket already connected",
                 NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "bind: unknown error", NULL, NULL);
    }
  }

  return out;
}

int sw_listen(int socket, int backlog) {
  int out = listen(socket, backlog);

  if (out == -1) {
    switch(errno) {
      case EBADF:
        SIG_SEND(SIGNAL_BAD_FILE_DESCRIPTOR, "listen(): Bad file descriptor", NULL, NULL);
        break;
      case EDESTADDRREQ:
        SIG_SEND(SIGNAL_UNSUPPORTED, "listen(): Socket not bound to a local address", NULL, NULL);
        break;
      case EINVAL:
        SIG_SEND(SIGNAL_INVALID_INPUT, "listen(): Socket already connected or has been shut down",
                 NULL, NULL);
        break;
      case ENOTSOCK:
        SIG_SEND(SIGNAL_NOT_SOCKET, "listen(): Socket argument does not refer to socket", NULL, NULL);
        break;
      case EOPNOTSUPP:
        SIG_SEND(SIGNAL_UNSUPPORTED, "listen(): Socket protocol does not support listen()", NULL, NULL);
        break;
      case EACCES:
        SIG_SEND(SIGNAL_PERMISSION_DENIED, "listen(): Permission denied", NULL, NULL);
        break;
      case ENOBUFS:
        SIG_SEND(SIGNAL_INSUFFICIENT_RESOURCES, "listen(): Insufficient resources", NULL, NULL);
        break;
      default:
        SIG_SEND(SIGNAL_UNKNOWN_ERROR, "listen(): unknown error", NULL, NULL);
        break;
    }
  }

  return out;
}
