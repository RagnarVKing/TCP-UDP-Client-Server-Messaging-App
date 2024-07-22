#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.hpp"
#include "helpers.hpp"

void run_client(int sockfd)
{
	char buf[MSG_MAXSIZE + 1];
	memset(buf, 0, MSG_MAXSIZE + 1);

	struct chat_packet sent_packet;
	struct chat_packet recv_packet;

	struct pollfd fds[2];
	int rc;
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;

	while (1) {
		rc = poll(fds, 2, -1);
		DIE(rc < 0, "poll");

		if (fds[0].revents & POLLIN) {
			int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
			if (rc <= 0)
				break;

			udp_msg *msg = (udp_msg *)recv_packet.message;

			if (msg->type == 0) {
				uint8_t byte;
				memcpy(&byte, msg->value, sizeof(uint8_t));
				uint32_t value;
				memcpy(&value, msg->value + 1, sizeof(uint32_t));
				value = ntohl(value);

				if (byte == 1)
					value = -value;

				printf("%s - INT - %d\n", msg->topic, value);
			}

			if (msg->type == 1) {
				uint16_t value;
				memcpy(&value, msg->value, sizeof(uint16_t));
				value = ntohs(value);
				printf("%s - SHORT_REAL - %.2f\n", msg->topic, (float)value / 100);
			}

			if (msg->type == 2) {
				uint8_t byte;
				memcpy(&byte, msg->value, sizeof(uint8_t));

				uint32_t concat;
				memcpy(&concat, msg->value + 1, sizeof(uint32_t));

				uint8_t power;
				memcpy(&power, msg->value + 5, sizeof(uint8_t));

				float value = ntohl(concat);

				if (byte == 1)
					value = -value;

				float pow = 1;
				for (int i = 0; i < power; i++)
					pow = pow * 10;

				value = value / pow;

				printf("%s - FLOAT - %.4f\n", msg->topic, value);
			}

			if (msg->type == 3)
				printf("%s - STRING - %s\n", msg->topic, (char *)msg->value);
		}

		if (fds[1].revents & POLLIN) {
			fgets(buf, sizeof(buf), stdin);
			if (isspace(buf[0]))
				break;

			buf[strlen(buf) - 1] = '\0';

			if (strncmp(buf, "exit", 4) == 0) {
				close(sockfd);
				break;
			} else if (strncmp(buf, "subscribe", 9) == 0) {
				char topic[55];
				rc = sscanf(buf, "%*s %s", topic);
				DIE(rc != 1, "sscanf");

				printf("Subscribed to topic %s\n", topic);
			} else if (strncmp(buf, "unsubscribe", 11) == 0) {
				char topic[55];
				rc = sscanf(buf, "%*s %s", topic);
				DIE(rc != 1, "sscanf");

				printf("Unsubscribed from topic %s\n", topic);
			} else 
        return;

			sent_packet.len = strlen(buf) + 1;
			strcpy(sent_packet.message, buf);

			send_all(sockfd, &sent_packet, sizeof(sent_packet));
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 4) {
		printf("\n Usage: %s <ip> <port>\n", argv[0]);
		return 1;
	}

  // disable buffering for stdout
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	struct chat_packet packet;
	packet.len = strlen(argv[1]);
	strcpy(packet.message, argv[1]);

	send_all(sockfd, &packet, sizeof(packet));

	run_client(sockfd);

	return 0;
}
