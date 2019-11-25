/*
 * main.c
 *
 *  Created on: Nov 22, 2019
 *      Author: sdragoi
 */


#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), connect(), recv() and send() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE				100
#define MAX_IP_LENGTH			15
#define MAX_FILENAME_LENGTH		32
#define FILE_NOT_FOUND_MSG		"File not found"
#define RECV_TIMEOUT_SEC		1
#define RECV_TIMEOUT_USEC		0

int main (int argc, char **argv)
{
	char buffer[100], filename[MAX_FILENAME_LENGTH + 1], server_ip[MAX_IP_LENGTH + 1];
	int running;
	int n_bytes, status;
	int fd, sock;
	short port;
	unsigned int addr_len;

	struct sockaddr_in server_addr;
	struct timeval timeout;

	if (argc != 4) {
		printf("Usage ./client server_ip server_port filename\n");
		return 0;
	} else {
		/* TODO Validate IP */
		if (strlen(argv[1]) > MAX_IP_LENGTH) {
			printf("Bad IP address\n");
			return 0;
		}
		memset(server_ip, 0, sizeof(server_ip));
		memcpy(server_ip, argv[1], strlen(argv[1]));

		status = sscanf(argv[2], "%hd", &port);
		if (status == EOF) {
			printf("Wrong port\n");
			return 0;
		}

		if (strlen(argv[3]) > MAX_FILENAME_LENGTH) {
			printf("File name too long\n");
			return 0;
		}
		memset(filename, 0, sizeof(filename));
		memcpy(filename, argv[3], strlen(argv[3]) + 1); /* Copy the '\0' as well */

		sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0) {
			printf("Unable to create socket\n");
			return 0;
		}
		timeout.tv_sec = RECV_TIMEOUT_SEC;
		timeout.tv_usec = RECV_TIMEOUT_USEC;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

		/* Set server address */
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		server_addr.sin_addr.s_addr = inet_addr(server_ip); /* Bind to the default IP address */

		addr_len = sizeof(server_addr);
		status = connect(sock, (struct sockaddr*)&server_addr, addr_len);
		if (status < 0) {
			printf("Client unable to connect\n");
			return 0;
		}

		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, filename, strlen(filename) + 1);
		n_bytes = send(sock, buffer, strlen(buffer) + 1, 0);
		if (n_bytes != strlen(buffer) + 1) {
			printf("Send failed\n");
			return 0;
		}

		running = 1;
		memset(buffer, 0, sizeof(buffer));
		n_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
		if (n_bytes <= 0) {
			printf("Got nothing from server after %d sec %d u_sec\n", RECV_TIMEOUT_SEC, RECV_TIMEOUT_USEC);
			running = 0;
		} else if (n_bytes == (strlen(FILE_NOT_FOUND_MSG) + 1)) {
			if (strncmp(buffer, FILE_NOT_FOUND_MSG, strlen(FILE_NOT_FOUND_MSG)) == 0) {
				printf("File not found...\n");
				running = 0;
			}
		} else {
			fd = creat(filename, 0600);
			if (fd < 0) {
				printf("Unable to create file\n");
				running = 0;
			}
		}

		while (running) {
			if (n_bytes > 0) {
				write(fd, buffer, n_bytes);
			} else {
				running = 0;
			}
			n_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
		}

		status = close(sock);
		if (status < 0) {
			printf("Error closing socket\n");
		}
	}
	return 0;
}
