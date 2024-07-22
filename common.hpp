#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

#define MSG_MAXSIZE 2048
#define MAX_CONNECTIONS 32
#define STRING_SIZE 256

struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE + 1];
};

#endif
