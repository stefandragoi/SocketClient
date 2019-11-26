/*
 * main.c
 *
 *  Created on: Nov 22, 2019
 *      Author: sdragoi
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE				25
#define MAX_IP_LENGTH			15
#define MAX_FILENAME_LENGTH		16
#define TIMEOUT_SEC				1
#define TIMEOUT_USEC			0
#define FILE_NOT_FOUND_MSG		"File not found"

int wildcard_check(char string[]) {
	int i, len;
	len = strlen(string);

	for (i = 0; i < len; i++) {
		switch(string[i]) {
		case '?':
		case '*':
		case '[':
		case ']':
			return 0;
		default:
			continue;
		}
	}

	return 1;
}

int main (int argc, char **argv)
{
	char buffer[BUFFER_SIZE], filename[MAX_FILENAME_LENGTH + 1], server_ip[MAX_IP_LENGTH];
	int i, running;
	int n_bytes, status;
	int fd, sock;
	short port;
	unsigned int addr_len;

	struct sockaddr_in server_addr;
	struct in_addr struct_addr;
	struct timeval timeout;

	if (argc != 4) {
		printf("Usage ./client server_ip server_port filename\n");
		return 0;
	} else {
		/* Check IP address, the correct format will be verified below by inet_pton function */
		if (strlen(argv[1]) > MAX_IP_LENGTH) {
			printf("Bad IP address\n");
			return 0;
		}
		memset(server_ip, 0, sizeof(server_ip));
		memcpy(server_ip, argv[1], strlen(argv[1]));

		/* check port format */
		for (i = 0; i < strlen(argv[2]); i++) {
			if (argv[2][i] < '0' || argv[2][i] > '9') {
				printf("Wrong port format\n");
				return 0;
			}
		}
		status = sscanf(argv[2], "%hd", &port);
		if (status == EOF) {
			printf("sscanf port error\n");
			return 0;
		}

		/* check file name */
		if (strlen(argv[3]) > MAX_FILENAME_LENGTH) {
			printf("File name too long\n");
			return 0;
		}
		memset(filename, 0, sizeof(filename));
		strncpy(filename, argv[3], strlen(argv[3]));
		if (0 == wildcard_check(filename)) {
			printf("Forbidden character detected in file name\n");
			return 0;
		}

		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0) {
			printf("Unable to create socket\n");
			return 0;
		}

		/* Set receiving timeout */
		timeout.tv_sec = TIMEOUT_SEC;
		timeout.tv_usec = TIMEOUT_USEC;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		/* Convert string address to binary format */
		status = inet_pton(AF_INET, server_ip, &struct_addr);
		if (status <= 0) {
			printf("Wrong IP address\n");
			return 0;
		}
		/* Set server address */
		server_addr.sin_addr = struct_addr;

		addr_len = sizeof(server_addr);
		status = connect(sock, (struct sockaddr*)&server_addr, addr_len);
		if (status < 0) {
			printf("Client unable to connect\n");
			return 0;
		}

		/* Send to server the name of the desired file */
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, filename, strlen(filename) + 1);
		n_bytes = send(sock, buffer, strlen(buffer) + 1, 0);
		if (n_bytes != strlen(buffer) + 1) {
			printf("Send failed\n");
			return 0;
		}

		/* Check what server sent */
		running = 1;
		memset(buffer, 0, sizeof(buffer));
		n_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
		if (n_bytes <= 0) {
			printf("Received nothing from server after %d sec %d u_sec\n", TIMEOUT_SEC, TIMEOUT_USEC);
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
