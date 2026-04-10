#pragma once
#include <sys/socket.h>

int sw_socket(int domain, int type, int protocol);
int sw_setsockopt(int socket,
                  int level,
                  int option_name,
                  const void* option_value,
                  socklen_t option_len);
int sw_bind(int socket, const struct sockaddr* address, socklen_t address_len);
int sw_listen(int socket, int backlog);
int sw_connect(int socket, const struct sockaddr* address, socklen_t address_len);
