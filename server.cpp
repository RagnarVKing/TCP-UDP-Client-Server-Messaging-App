#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "common.hpp"
#include "helpers.hpp"

vector < sub > subs;

int my_strcmp(char *s1, char *s2)
{
	if (strcmp(s1, s2) == 0)
		return 0;

	if (strcmp(s1, "*") == 0)
		return 0;

	if (strcmp(s2, "*") == 0)
		return 0;

	char first[STRING_SIZE];
	strcpy(first, s1);
	char *token1 = strtok(first, "/");

	vector < char * > v1;
	while (token1) {
		v1.push_back(strdup(token1));
		token1 = strtok(NULL, "/");
	}

	char second[STRING_SIZE];
	strcpy(second, s2);
	char *token2 = strtok(second, "/");

	vector < char * > v2;
	while (token2) {
		v2.push_back(strdup(token2));
		token2 = strtok(NULL, "/");
	}

	size_t i = 0, j = 0;

	for (; i < v1.size() && j < v2.size(); i++, j++) {
		if (strcmp(v1[i], v2[j]) == 0)
			continue;

	  if (strcmp(v1[i], "+") == 0)
		  continue;

	  if (strcmp(v2[j], "+") == 0)
		  continue;

	  if (strcmp(v1[i], "*") == 0) {
		  if (i == v1.size() - 1)
			  return 0;

		  i++;
		  while (strcmp(v1[i], v2[j]) != 0) {
			  j++;
			  if (j == v2.size())
				  return 1;
      }
			continue;
	  }

	  if (strcmp(v2[j], "*") == 0) {
		  if (j == v2.size() - 1)
			  return 0;

		  j++;
		  while (strcmp(v1[i], v2[j]) != 0) {
			  i++;
			  if (i == v1.size())
				  return 1;
		  }
      continue;
    }

		return 1;
  }
	
  if (i == v1.size() && j == v2.size())
		return 0;

	return 1;
}

void run_chat_multi_server(int listenfd, int sockfd)
{
	vector < pollfd > poll_fds;
	int num_sockets = 3;
	int rc;

	struct chat_packet received_packet;

	rc = listen(listenfd, SOMAXCONN);
	DIE(rc < 0, "listen");

	struct pollfd p;
	p.fd = listenfd;
	p.events = POLLIN;
	poll_fds.push_back(p);

	p.fd = STDIN_FILENO;
	poll_fds.push_back(p);

	p.fd = sockfd;
	poll_fds.push_back(p);

	while (1) {
		rc = poll(poll_fds.data(), num_sockets, -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < num_sockets; i++) {
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == listenfd) {
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					const int newsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");

					int rc = recv_all(newsockfd, &received_packet, sizeof(received_packet));
					DIE(rc < 0, "recv");

					int ok = 0;
					for (size_t j = 0; j < subs.size(); j++) {
						if (strcmp(received_packet.message, subs[j].id) == 0) {
							if (subs[j].on == 0) {
								subs[j].on = 1;
								subs[j].sockfd = newsockfd;
								printf("New client %s connected from %s:%d.\n", received_packet.message, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
							} else {
								printf("Client %s already connected.\n", subs[j].id);
								poll_fds[num_sockets - 1].revents = 0;
								close(newsockfd);
							}
							ok = 1;
							break;
						}
					}

					if (ok == 1)
						continue;

					struct sub new_sub;
					new_sub.sockfd = newsockfd;
					new_sub.id = strdup(received_packet.message);
					new_sub.on = 1;
					subs.push_back(new_sub);

					p.fd = newsockfd;
					poll_fds.push_back(p);

					num_sockets++;

					printf("New client %s connected from %s:%d.\n", received_packet.message, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

				} else if (poll_fds[i].fd == STDIN_FILENO) {
					char buf[MSG_MAXSIZE + 1];
					fgets(buf, MSG_MAXSIZE, stdin);
					buf[strlen(buf) - 1] = '\0';

					if (strcmp(buf, "exit") == 0) {
						for (int j = 2; j < num_sockets; j++)
							close(poll_fds[j].fd);
						return;
					} else
						return;

				} else if (poll_fds[i].fd == sockfd) {
					char buf[1552];
					int rc = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);
					buf[rc] = '\0';

					udp_msg *msg = (udp_msg *)buf;

					memcpy(received_packet.message, buf, sizeof(udp_msg));
					received_packet.len = sizeof(udp_msg);

					for (size_t j = 0; j < subs.size(); j++) {
						if (subs[j].on == 1) {
							for (size_t k = 0; k < subs[j].topics.size(); k++) {
								if (my_strcmp(subs[j].topics[k], msg->topic) == 0) {
									int rc = send_all(subs[j].sockfd, &received_packet, sizeof(received_packet));
									DIE(rc < 0, "send");
									break;
								}
							}
						}
					}
				} else {
					int rc = recv_all(poll_fds[i].fd, &received_packet, sizeof(received_packet));
					DIE(rc < 0, "recv");

					if (rc == 0) {
						for (size_t j = 0; j < subs.size(); j++) {
							if (subs[j].sockfd == poll_fds[i].fd) {
								subs[j].on = 0;
								printf("Client %s disconnected.\n", subs[j].id);
								break;
							}
						}

						poll_fds[i].revents = 0;
						close(poll_fds[i].fd);

						poll_fds.erase(poll_fds.begin() + i);

						num_sockets--;
						i--;
					} else {
						char *s = strtok(received_packet.message, " ");
						int type;
						char *topic;
						if (strcmp(s, "subscribe") == 0)
							type = 1;
						else if (strcmp(s, "unsubscribe") == 0)
							type = 0;
						else
							return;

						topic = strtok(NULL, "\n");

						for (size_t j = 0; j < subs.size(); j++) {
							if (subs[j].sockfd == poll_fds[i].fd) {
								if (type == 1) {
									subs[j].topics.push_back(strdup(topic));
								} else {
									for (size_t k = 0; k < subs[j].topics.size(); k++) {
										if (strcmp(subs[j].topics[k], topic) == 0) {
											subs[j].topics.erase(subs[j].topics.begin() + k);
											break;
										}
									}
								}
								break;
							}
						}
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("\n Usage: %s <ip> <port>\n", argv[0]);
		return 1;
	}

	// disable buffering for stdout
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listenfd < 0, "socket");

	// disable Nagle's algorithm
	int flag = 1;
	rc = setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(int));

	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	const int enable = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bind");

	const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd < 0, "socket");

	struct sockaddr_in udp_addr;
	socklen_t udp_len = sizeof(struct sockaddr_in);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	memset(&udp_addr, 0, udp_len);
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	rc = bind(sockfd, (const struct sockaddr *)&udp_addr, sizeof(udp_addr));
	DIE(rc < 0, "bind");

	run_chat_multi_server(listenfd, sockfd);

	return 0;
}
