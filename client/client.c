/**
* @file client.c
* @author Vivien Bihl
* @version 1.0
*
* @section LICENSE
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details at
* http://www.gnu.org/copyleft/gpl.html
*
* @section DESCRIPTION
* Multi user chat room client
* ./client IPv6_addr port_number string
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NAMESIZ 24
#define MSGSIZ 1024

int main(int argc, char **argv)
{
	int sockfd;
	int addrlen;
	int fdmax;
	char buf[BUFSIZ];
	char message[MSGSIZ];
	struct sockaddr_in6 server;
	fd_set master;	
	fd_set read_fds;
	
	// check the number of args on command line
	if(argc != 4)
	{
		printf("USAGE: %s @server port_num nickname\n", argv[0]);
		exit(-1);
	}

	// socket factory
	if((sockfd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		perror("error socket");
		exit(EXIT_FAILURE);
	}


	// init remote addr structure and other params
	server.sin6_family = AF_INET6;
	server.sin6_port   = htons(atoi(argv[2]));
	addrlen           = sizeof(struct sockaddr_in6);

	// get addr from command line and convert it
	if(inet_pton(AF_INET6, argv[1], &(server.sin6_addr)) != 1)
	{
		perror("error inet_pton");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	printf("Trying to connect to the remote host\n");
	if(connect(sockfd, (struct sockaddr*)&server, addrlen) == -1)
	{
		perror("error connect");
		exit(EXIT_FAILURE);
	}
	printf("Connection OK\n");
	
	// send nickname
	if(write(sockfd, argv[3], NAMESIZ) == -1)
	{
		perror("error send");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	
	FD_SET(STDIN_FILENO, &master); // add standard input to the set 
	FD_SET(sockfd, &master); // add socket to the set

	// keep track of the biggest file descriptor	
	fdmax = STDIN_FILENO + 1;

	if (sockfd > fdmax) {
		fdmax = sockfd + 1;
	}

	while(strncmp(message, "quit", 4) != 0)
	// main loop
	{
		read_fds = master;
		
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			close(sockfd);
		}

		// event on socket
		if (FD_ISSET(sockfd, &read_fds)) {
			if (read(sockfd, buf, BUFSIZ) == -1) {
				perror("recv message");
			}
			printf("%s", buf);
			memset(buf, 0, BUFSIZ); //erase previous content
		}
		// input from user	
		if(FD_ISSET(STDIN_FILENO, &read_fds))
		{
			memset(message, 0, MSGSIZ); //erase previous content in case new message is shorter than previous one
			//fgets(message, MSGSIZ, stdin);
			read(STDIN_FILENO,message,MSGSIZ);

			if(write(sockfd, message, MSGSIZ) == -1)
			{
				perror("error send");
				close(sockfd);
				exit(EXIT_FAILURE);
			}
		}
	}

	printf("Disconnection\n");
	close(sockfd);

	return 0;
}
