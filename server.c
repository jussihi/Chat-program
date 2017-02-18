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

#define MAXCLIENT 40
#define BUFFER 1000

typedef struct
{
	char name[20];
	int userid;
	struct sockaddr_in addr;
	int clientfd;
}client;

client* clientarr[MAXCLIENT];

static int clients = 0;
static int userid = 100;
static int port = 5555;

//add a new client to the clientarr-array
client* add_client(client* newclient)
{
	int i = 0;
	for(; i< MAXCLIENT; i++)
	{
		if(clientarr[i] == NULL)
		{
			clientarr[i] = newclient;
			return clientarr[i];
		}
	}
	return NULL;
}

//broadcast a message to every client
void send_all(char* msg)
{
	int i = 0;
	for(; i < MAXCLIENT; i++)
	{
		if(clientarr[i] != NULL)
		{
			write(clientarr[i]->clientfd, msg, strlen(msg));
		}
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
		sprintf(outbuf, "%s:\t%s\n",connclient->name, inbuf);
		send_all(outbuf);
	}
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
		client* newclient = malloc(sizeof(client));
		newclient->userid = userid;
		newclient->clientfd = acctfd;
		newclient->addr = clientaddr;
		sprintf(newclient->name, "%d", newclient->userid);
		userid++;
		clients++;
		printf("Creating new thread for client with user id %d\n", (newclient->userid));
		if(!(add_client(newclient))) printf("ERROR WHEN ADDING USER TO CLIENTARRAY\n");
		pthread_create(&pth, NULL, (void *)&connection_handler, newclient);
		sleep(1);
	}
	close(acctfd);
}

int main()
{
	int sockfd;
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
		else printf("invalid input.");
	} while(1);

	pthread_cancel(pts);
	close(sockfd);
	printf("Server closed.\n");
	return 0;
}
