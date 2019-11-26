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
#define BAD_FILE_MSG			"Bad file name"
#define FILE_NOT_FOUND_MSG		"File not found"
#define SERVER_ERROR_MSG		"Server reading error"

int wildcard_check(char string[]) {
	int i, len;
	len = strlen(string);

	for (i = 0; i < len; i++) {
		switch(string[i]) {
		case '?':
		case '*':
		case '[':
		case ']':
		case '^':
			return 0;
		default:
			continue;
		}
	}

	return 1;
}

short get_port(char string[]) {
	short port;
	int i, len, status;

	len = strlen(string);
	for (i = 0; i < len; i++) {
		if (string[i] < '0' || string[i] > '9') {
			printf("Wrong port format\n");
			return -1;
		}
	}
	status = sscanf(string, "%hd", &port);
	if (status == EOF) {
		printf("sscanf port error\n");
		return -1;
	}

	return port;
}

void process_data(int sock, char filename[]) {
	int fd;
	int n_bytes;
	char buffer[BUFFER_SIZE];

	/* Check for any error message, otherwise create the file */
	n_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
	if (n_bytes <= 0) {
		printf("Received nothing from server after %d sec %d u_sec\n", TIMEOUT_SEC, TIMEOUT_USEC);
		return;
	} else if (n_bytes == (strlen(FILE_NOT_FOUND_MSG) + 1)) {
		if (strncmp(buffer, FILE_NOT_FOUND_MSG, strlen(FILE_NOT_FOUND_MSG)) == 0) {
			printf("File not found...\n");
			return;
		}
	} else if (n_bytes == (strlen(BAD_FILE_MSG) + 1)) {
		if (strncmp(buffer, BAD_FILE_MSG, strlen(BAD_FILE_MSG)) == 0) {
			printf("Name contains forbidden characters.\n");
			return;
		}
	} else if (n_bytes == (strlen(SERVER_ERROR_MSG) + 1)) {
		if (strncmp(buffer, FILE_NOT_FOUND_MSG, strlen(SERVER_ERROR_MSG)) == 0) {
			printf("Server unable to read file.\n");
			return;
		}
	} else {
		fd = creat(filename, 0600);
		if (fd < 0) {
			printf("Unable to create file\n");
			return;
		}
	}

	/* Write in file until server stops sending data */
	while (1) {
		if (n_bytes > 0) {
			write(fd, buffer, n_bytes);
		} else {
			return;
		}
		n_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
	}
}

int main (int argc, char **argv)
{
	char buffer[BUFFER_SIZE], filename[MAX_FILENAME_LENGTH + 1], server_ip[MAX_IP_LENGTH];
	int n_bytes, status;
	int sock;
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

		port = get_port(argv[2]);
		if (port < 0) {
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

		/* Create client socket */
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock < 0) {
			printf("Unable to create socket\n");
			return 0;
		}
		/* Set receiving timeout */
		timeout.tv_sec = TIMEOUT_SEC;
		timeout.tv_usec = TIMEOUT_USEC;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

		/* Connect to server */
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		status = inet_pton(AF_INET, server_ip, &struct_addr); /* Convert string address to binary format */
		if (status <= 0) {
			printf("Wrong IP address\n");
			return 0;
		}
		server_addr.sin_addr = struct_addr; /* Set server address */
		addr_len = sizeof(server_addr);
		status = connect(sock, (struct sockaddr*)&server_addr, addr_len);
		if (status < 0) {
			printf("Client unable to connect\n");
			return 0;
		}

		/* Send the name of the desired file */
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, filename, strlen(filename) + 1);
		n_bytes = send(sock, buffer, strlen(buffer) + 1, 0);
		if (n_bytes != strlen(buffer) + 1) {
			printf("Send failed\n");
			return 0;
		}

		/* Process what server sent */
		process_data(sock, filename);

		status = close(sock);
		if (status < 0) {
			printf("Error closing socket\n");
		}
	}
	return 0;
}
