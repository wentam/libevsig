#include "libevsig/signals.h"
#include "libevsig/errno_signals.h"
#include <sys/socket.h>
#include <errno.h>

int sw_socket(int domain, int type, int protocol) {
  int out = socket(domain, type, protocol);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("socket(): ", errno),
             NULL, NULL);

  return out;
}

int sw_setsockopt(int socket,
                  int level,
                  int option_name,
                  const void* option_value,
                  socklen_t option_len) {
  int out = setsockopt(socket, level, option_name, option_value, option_len);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("setsockopt(): ", errno),
             NULL, NULL);

  return out;
}

int sw_bind(int socket, const struct sockaddr* address, socklen_t address_len) {
  int out = bind(socket, address, address_len);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("bind(): ", errno),
             NULL, NULL);

  return out;
}

int sw_listen(int socket, int backlog) {
  int out = listen(socket, backlog);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("listen(): ", errno),
             NULL, NULL);

  return out;
}


int sw_connect(int socket, const struct sockaddr* address, socklen_t address_len) {
  int out = connect(socket, address, address_len);

  if (out == -1)
    SIG_SEND(sig_from_errno(errno), str_from_errno("connect(): ", errno),
             NULL, NULL);

  return out;
}
