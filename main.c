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
#define END_SIGNAL_MSG			"the end"

/* I am the client */

int main (int argc, char **argv)
{
	char buffer[100], filename[MAX_FILENAME_LENGTH], server_ip[MAX_IP_LENGTH];
	int n_bytes, status;
	int fd, sock;
	short port;
	unsigned int addr_len;

	struct sockaddr_in server_addr;


	if (argc != 4) {
		printf("Usage ./client server_ip server_port filename\n");
		return 0;
	} else {
		/* TODO Validate IP */
		if (strlen(argv[1]) > MAX_IP_LENGTH) {
			printf("Bad IP address\n");
			return 0;
		}
		memcpy(server_ip, argv[1], strlen(argv[1]));
		sscanf(argv[2], "%hd", &port);

		if (strlen(argv[3]) > MAX_FILENAME_LENGTH) {
			printf("File name too long\n");
			return 0;
		}
		memcpy(filename, argv[3], strlen(argv[3]));
		fd = creat(filename, 0600);
		if (fd < 0) {
			printf("Unable to create file\n");
			return 0;
		}

		sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

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

		strncpy(buffer, filename, strlen(filename));
		n_bytes = send(sock, buffer, strlen(buffer), 0);
		if (n_bytes != strlen(buffer)) {
			printf("Send failed\n");
			// return 0;
		}
		memset(buffer, 0 , sizeof(buffer));

		n_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
		int running = 1;
		while (running) {
			n_bytes = recv(sock, buffer, BUFFER_SIZE, 0);
			printf("client got: %s\n", buffer);
			if (BUFFER_SIZE == n_bytes) {
				write(fd, buffer, n_bytes);
			} else if (BUFFER_SIZE > n_bytes) {
				if (strncmp(buffer, END_SIGNAL_MSG, strlen(END_SIGNAL_MSG)) == 0) {
					running = 0;
				} else {
					write(fd, buffer, n_bytes);
				}
			}
		}

		status = close(sock);
		if (status < 0) {
			printf("Error closing server socket\n");
		}
	}
	return 0;
}
