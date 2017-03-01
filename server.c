#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include "server.h"

#define MAXCLIENT 40
#define BUFFER 1000


static int clients = 0;
static int userid = 100;
static int port = 5555;
clientList cl;


client* clientList_add(int clientfd, struct sockaddr_in clientaddr)
{
	client* newclient = malloc(sizeof(client));
	if(!newclient)
	{
		return NULL;
		printf("ERROR WHEN ADDING USER TO CLIENTARRAY\n");
	}
	newclient->userid = userid;
	newclient->clientfd = clientfd;
	newclient->addr = clientaddr;
	newclient->next = NULL;
	sprintf(newclient->name, "%d", newclient->userid);
	userid++;
	clients++;
	printf("Creating new thread for client with user id %d\n", (newclient->userid));
	if (cl.last)
		cl.last->next = newclient;
	cl.last = newclient;
	if (!cl.first)
	cl.first = cl.last;
	return newclient;
}

void clientList_drop(int id)
{
	client* c = cl.first;
	client* p = NULL;
	while (c)
	{
		if (c->userid == id)
		{
		    if (p)
		        p->next = c->next;
		    if (c == cl.first)
		        cl.first = c->next;
		    if (p && p->next == NULL)
		        cl.last = p;
		    free(c);
		    return;
		}
		p = c;
		c = c->next;
	}
	return;
}


void send_whisper(char* msg, int send_uid, int recv_uid)
{
	return;
}

void send_priv_serv(char* msg, int uid)
{
	for(client* tmp = cl.first; tmp!=NULL; tmp = tmp->next)
	{
		if(tmp->userid == uid)
		{
			write(tmp->clientfd, msg, strlen(msg));
		}
	}
}



//broadcast a message to every client
void send_all(char* msg)
{
	for(client* tmp = cl.first; tmp != NULL; tmp = tmp->next)
	{
		write(tmp->clientfd, msg, strlen(msg));
	}
}


void* connection_handler(client* connclient)
{
	char outbuf[BUFFER];
	char inbuf[BUFFER];

	int n;
	while((n = read(connclient->clientfd, inbuf, sizeof(inbuf) - 1)) > 0)
	{
		inbuf[n] = 0;
		if(strncmp(inbuf, "/name ", 6) == 0)
		{
			char oldname[20];
			strncpy(oldname, connclient->name, 19);
			oldname[19] = 0;
			memset(connclient->name, 0, 20);
			strncpy(connclient->name, (inbuf+6), 19);
			sprintf(outbuf, "%s changed name to %s\n",oldname, connclient->name);
			printf("%s", outbuf);
		}
		else
		{
			sprintf(outbuf, "%s:\t%s\n",connclient->name, inbuf);
		}
		send_all(outbuf);
		printf("asd\n");
	}
	close(connclient->clientfd);
	printf("%s closed connection.\n", connclient->name);
	clientList_drop(connclient->userid);
	clients--;
	return NULL;
}


void* socket_initializer(int* sockfd)
{
	int acctfd;
	struct sockaddr_in clientaddr;
	pthread_t pth;

	while(1)
	{
		socklen_t clientlen = sizeof(clientaddr);
		acctfd = accept(*sockfd, (struct sockaddr*)&clientaddr, &clientlen);
		if(clients == MAXCLIENT - 1)
		{
			printf("Maximum number of clients reached, dropping new connection.\n");
			close(acctfd);
			continue;
		}
		client* newclient = clientList_add(acctfd, clientaddr);
		pthread_create(&pth, NULL, (void *)&connection_handler, newclient);
		sleep(1);
	}
	close(acctfd);
}

int main()
{
	int sockfd;
	cl.first = NULL;
	cl.last = NULL;
	struct sockaddr_in servaddr;
	pthread_t pts;
	char input[100];
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
        	perror("socket error");
        	return 1;
    	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	
	if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("socket binding error");
		return 1;
	}

	if(listen(sockfd, 20) < 0)
	{
		perror("socket listening error");
		return 1;
	}
	
	pthread_create(&pts, NULL, (void *)&socket_initializer, &sockfd);
	printf("SERVER CREATION SUCCESSFUL. For help, type 'help' and press ENTER.\n");
	do
	{
		scanf("%s", (char*)&input);
		if(strstr(input, "help") != NULL) printf("Possible commands:\nhelp\tShow this help screen\nstatus\tShow server status\nquit\tClose the server\n");
		else if(strstr(input, "status") != NULL) printf("Current users: %d\nRunning on port: %d\n", clients, port);
		else if(strstr(input, "quit") != NULL) break;
		else printf("invalid input.\n");
	} while(1);

	pthread_cancel(pts);
	close(sockfd);
	printf("Server closed.\n");
	return 0;
}
