#ifndef _HELPERS_HPP
#define _HELPERS_HPP 1

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <regex>

using namespace std;

struct sub {
  int sockfd = -1;
  char *id;
  vector<char *> topics;
  bool on;
};

struct udp_msg {
  char topic[50];
  uint8_t type;
  char value[1501];
};

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif
