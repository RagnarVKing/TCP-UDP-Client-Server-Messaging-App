#include "common.hpp"

#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;

    while(bytes_remaining) {
      int rc = recv(sockfd, buff, bytes_remaining, 0);
      if (rc == 0) {
        return rc;
      }
      bytes_received += rc;
      bytes_remaining -= rc;
      buff += rc;
    }

  return recv(sockfd, buffer, len, 0);
}


int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;
  
    while(bytes_remaining) {
      int rc = send(sockfd, buff, bytes_remaining, 0);
      if (rc == 0) {
        return rc;
      }
      bytes_sent += rc;
      bytes_remaining -= rc;
      buff += rc;
    }
  
  return send(sockfd, buffer, len, 0);
}
