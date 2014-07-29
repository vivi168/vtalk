/**
* @file server.c
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
* ./server port_number
* Multi users chat room server
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXCON 12 // number max of clients
#define NAMESIZ 24 // max name length
#define MSGSIZ 1024 // message size

struct clientinfo{
	char name[NAMESIZ]; // client nickname
	int sd; // socket
	int free; // boolean to know if we are dealing with an existing connection or not
};

int main(int argc, char **argv)
{
	int sockfd; // listener
	int sockfd2; // accept
	int i, j, k; // for iterators
	int nbytes; // size of the received message, if 0 -> EOF notif
	int fdmax;
	int found = 0; // if a free slot is found
	socklen_t addrlen;
	char buf[BUFSIZ];
	char msgbuf[MSGSIZ];
	char namebuf[NAMESIZ];
	struct sockaddr_in my_addr;
	struct sockaddr_in client;
	struct clientinfo ci[MAXCON]; // client slots
	fd_set master;
	fd_set read_fds;

	for(i = 0; i < MAXCON; i++)
	{
		ci[i].free = 1; // initialize all client slots as free
	}

	// check the number of args on command line
	if(argc != 2)
	{
		printf("USAGE: %s port_num\n", argv[0]);
		exit(-1);
	}

	if((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		perror("error socket");
		exit(EXIT_FAILURE);
	}

	my_addr.sin_family	= AF_INET;
	my_addr.sin_port = htons(atoi(argv[1]));
	my_addr.sin_addr.s_addr = INADDR_ANY;
	addrlen	= sizeof(struct sockaddr_in);
	memset(buf,'\0',BUFSIZ);

	if(bind(sockfd, (struct sockaddr*)&my_addr, addrlen) == -1)
	{
		perror("error bind");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	if(listen(sockfd, 10) == -1)
	{
		perror("error listen");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	//add the listener to the master set
	FD_SET(sockfd, &master);

	//keep track of the biggest file descriptor
	fdmax = sockfd;
	if(sockfd > fdmax)
	{
		fdmax = sockfd + 1;
	}

	//main loop
	while(1)
	{
		read_fds = master; //backup master

		if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			close(sockfd);
			close(sockfd2);
			exit(EXIT_FAILURE);
		}

		// new connection
		if(FD_ISSET(sockfd, &read_fds))
		{
			found = 0;

			if((sockfd2 = accept(sockfd, (struct sockaddr*)&client, &addrlen)) == -1)
			{
				perror("accept");
			}

			for(j = 0; j < MAXCON; j++)
			// cycles through every connection to see if there is a free one
			{
				if(ci[j].free == 1)
				// if it's free, then assign it
				{
					found = 1;
					break;
				}
			}

			if(found == 0)
			// if found == 0, it means server is full
			{
				printf("server full\n");
			}
			else
			{
				ci[j].free = 0;
				ci[j].sd = sockfd2;
				if(read(ci[j].sd, namebuf, NAMESIZ) == -1)
				{
					perror("recv name");
				}

				strcpy(ci[j].name, namebuf);
				FD_SET(ci[j].sd, &master);
				if(ci[j].sd > fdmax)
					fdmax = ci[j].sd + 1;

				printf("%s has connected\n", ci[j].name);
			}
		}

		//incoming data from clients
		for(i = 0; i < MAXCON; i++)
		{
			if(FD_ISSET(ci[i].sd, &read_fds))
			{
				if((nbytes = read(ci[i].sd, msgbuf, MSGSIZ)) == -1)
				{
					perror("recv msg");
				}
				if(nbytes == 0)
				// if read returns 0 then it means that client has disconnected
				{
					FD_CLR(ci[i].sd, &master);
					close(ci[i].sd);
					ci[i].free = 1;
					printf("%s has disconnected\n", ci[i].name);
				}
				else
				{
					for(k = 0; k < MAXCON; k++)
					// send received message to other cients
					{
						if(ci[k].free == 0 && ci[k].sd != ci[i].sd)
						// treat only occupied slots and don't send to self.
						{
							memset(buf, 0, BUFSIZ);
							sprintf(buf, "%s: %s", ci[i].name, msgbuf);
							if(write(ci[k].sd, buf, BUFSIZ) == -1)
							{
								perror("send msg");
							}
						}
					}
				}
			}
		}

	}
	// close sockets
	close(sockfd);
	close(sockfd2);
	return 0;
}
