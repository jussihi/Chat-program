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

#define BUFFER 1000

static int clients = 0;
static int userid = 100;
static int port = 5555;
clientList cl;

client* clientList_add(int clientfd, struct sockaddr_in clientaddr)
{
	client* newclient = calloc(1, sizeof(client));
	if(!newclient)
	{
		return NULL;
		printf("ERROR WHEN ADDING USER TO CLIENTARRAY\n");
	}
	newclient->userid = userid;
	newclient->clientfd = clientfd;
	newclient->addr = clientaddr;
	sprintf(newclient->name, "%d", newclient->userid);
	newclient->pthp = calloc(1, sizeof(pthread_t));
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
	void* ret = NULL;
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
			pthread_cancel(*(c->pthp));
			pthread_join(*(c->pthp), &ret);
			free(c->pthp);
			free(c);
			return;
		}
		p = c;
		c = c->next;
	}
	return;
}

void clientList_empty()
{
	void* ret = NULL;
	client* c = cl.first;
	client* p = NULL;
	while (c)
	{
		p = c;
		c = c->next;
		pthread_cancel(*(p->pthp));
		pthread_join(*(p->pthp), &ret);
		free(p->pthp);
		free(p);
	}
	return;
}


void send_whisper(char* msg, int send_uid, int recv_uid)
{
	return;
}

void send_userlist(client* connclient)
{
	char output[BUFFER];
	sprintf(output, "List of active users:\n");
	for(client* tmp = cl.first; tmp != NULL; tmp = tmp->next)
	{
		sprintf(output, "%s%s\n", output, tmp->name);
	}
	write(connclient->clientfd, output, strlen(output));
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
		else if(strncmp(inbuf, "/active", 7) == 0)
		{
			send_userlist(connclient);
			continue;
		}
		else
		{
			sprintf(outbuf, "%s:\t%s\n",connclient->name, inbuf);
		}
		send_all(outbuf);
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

	while(1)
	{
		//pthread_t* pthp = calloc(1, sizeof(pthread_t));
		printf("listening for thread\n");
		socklen_t clientlen = sizeof(clientaddr);
		acctfd = accept(*sockfd, (struct sockaddr*)&clientaddr, &clientlen);
		client* newclient = clientList_add(acctfd, clientaddr);
		pthread_create(newclient->pthp, NULL, (void *)&connection_handler, newclient);
		printf("creating new thread with &pthp = %p\n", newclient->pthp);
		printf("Calloced memory: %d\n", (int)*(newclient->pthp));
		if(!*(newclient->pthp)) free(newclient->pthp);
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
	void* ret = NULL;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
        	perror("socket error");
        	return 1;
    	}
	
	memset(&servaddr, 0, sizeof(servaddr));
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
	while(1)
	{
		scanf("%s", (char*)&input);
		if(strstr(input, "help") != NULL) printf("Possible commands:\nhelp\tShow this help screen\nstatus\tShow server status\nquit\tClose the server\n");
		else if(strstr(input, "status") != NULL) printf("Current users: %d\nRunning on port: %d\n", clients, port);
		else if(strstr(input, "quit") != NULL) break;
		else printf("invalid input.\n");
	}
	pthread_cancel(pts);
	pthread_join(pts, &ret);
	clientList_empty();
	close(sockfd);
	printf("Server closed.\n");
	return 0;
}
