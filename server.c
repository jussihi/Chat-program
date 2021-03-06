#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
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
channellList chl;
char* welcome = NULL;


/*
function to check for invalid char arrays
(arrays that contain non-alphanumeric chars)
*/
int check_str_valid(const char* str)
{
	int flag = 0;
	if(!str) return -1;
	for(int i = 0; i < strlen(str); i++)
	{
		if(!isalnum(str[i])) flag = 1;
	}
	return flag;
}

char* load_welcome(const char* filename)
{
	FILE *fp;
	long fsize;
	char *buffer;

	fp = fopen(filename, "r");
	if(!fp)
	{
		perror(filename);
		return NULL;
	}
	fseek(fp, 0L, SEEK_END);
	fsize = ftell(fp);
	rewind(fp);

	buffer = calloc( 1, (fsize+1) * sizeof(char));
	if(!buffer)
	{
		fclose(fp);
		fputs("memory alloc failed.",stderr);
		return NULL;
	}

	if(1 != fread(buffer, fsize, 1, fp))
	{
		fclose(fp);
		free(buffer);
		fputs("entire read fails",stderr);
		return NULL;
	}

	fclose(fp);
	return buffer;
}

channel* find_channel(const char* name)
{
	channel* ch = chl.first;
	while(ch)
	{
		if(strcmp(ch->name, name) == 0) return ch;
		ch = ch->next;
	}
	return NULL;
}

channel* init_channel(const char* name)
{
	channel* newchannel = calloc(1, sizeof(channel));
	if(!newchannel)
	{
		printf("ERROR WHEN ADDING CHANNEL TO CHANNEL ARRAY\n");
		return NULL;
	}
	strncpy(newchannel->name, name, 20);
	newchannel->name[20] = 0;
	newchannel->users = 0;
	if(chl.last)
		chl.last->next = newchannel;
	chl.last = newchannel;
	if(!chl.first)
		chl.first = chl.last;
	newchannel->clients = calloc(1, sizeof(client));
	newchannel->admins = calloc(1, sizeof(client));
	return newchannel;
}


int join_channel(const char* chn, client* cl)
{
	if(cl->channel)
	{
		send_priv_serv("You are already connected to a channel! Please use /leave\n", cl->userid);
		return 1;
	}
	channel* ch = find_channel(chn);
	if(!ch)
	{
		send_priv_serv("Channel does not exist. You can create such channel with /create\n", cl->userid);
		return 1;
	}
	int n = 0;
	client** p = ch->clients;
	while(*p)
	{
		p++;
	}
	n = p - ch->clients;
	ch->clients = realloc(ch->clients, (n+2) * sizeof(client*));
	ch->clients[n] = cl;
	ch->clients[n+1] = NULL;
	cl->channel = ch;
	return 0;
}

int leave_channel(client* cl)
{
	if(!cl->channel)
	{
		send_priv_serv("You are not connected to a channel!\n", cl->userid);
		return 1;
	}
	int n = 0;
	int j = -1;
	client** p = cl->channel->clients;
	while(*p)
	{
		p++;
		n++;
		if(*p == cl) j = (n-1);
	}
	if(j == -1)
	{
		send_priv_serv("Unknown error when exiting channel!\n", cl->userid);
		return 1;
	}
	client** h = realloc(cl->channel->clients, n * sizeof(client*));
	memcpy(h+j, cl->channel->clients + j + 1, (n-j)*sizeof(client*));
	free(cl->channel->clients);
	cl->channel->clients = h;
	cl->channel = NULL;
	return 0;
}

client* clientList_add(int clientfd, struct sockaddr_in6 clientaddr)
{
	client* newclient = calloc(1, sizeof(client));
	if(!newclient)
	{
		printf("ERROR WHEN ADDING USER TO CLIENT ARRAY\n");
		return NULL;
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

client* searchclient(const char* name)
{
	for(client* tmp = cl.first; tmp!=NULL; tmp = tmp->next)
	{
		if(strcmp(tmp->name, name) == 0)
		{
			printf("loytyi\n");
			return tmp;
		}
	}
	return NULL;
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
			//pthread_cancel(*(c->pthp)); //STILL LOSING 272 bytes memory per disconnected client
			pthread_join(*(c->pthp), &ret);  //joining thread inside itself doesn't seem to work
			free(c->pthp);
			free(c);
			clients--;
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

void channelList_empty()
{
	channel* c = chl.first;
	channel* p = NULL;
	while (c)
	{
		p = c;
		c = c->next;
		free(p->clients);
		free(p->admins);
		free(p);
	}
	return;
}


void send_whisper(char* msg, const char* sendname, client* recv)
{
	char outbuf[BUFFER];
	sprintf(outbuf, "%s (W):\t%s\n", sendname, msg);
	write(recv->clientfd, outbuf, strlen(outbuf));
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

void send_channellist(client* connclient)
{
	char output[BUFFER];
	sprintf(output, "List of active channels:\n");
	if(chl.first == NULL) sprintf(output, "%sNo active channels yet! Please make one.\n", output);
	for(channel* tmp = chl.first; tmp != NULL; tmp = tmp->next)
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

void send_all_channel(const char* msg, channel* ch)
{
	for(client* tmp = cl.first; tmp != NULL; tmp = tmp->next)
	{
		if(tmp->channel == ch) write(tmp->clientfd, msg, strlen(msg));
	}
}

void* connection_handler(client* connclient)
{
	char outbuf[BUFFER];
	char inbuf[BUFFER];
	int n;

	if(welcome) send_priv_serv(welcome, connclient->userid);

	while((n = read(connclient->clientfd, inbuf, sizeof(inbuf) - 1)) > 0)
	{
		inbuf[n] = 0;
		if(strncmp(inbuf, "/name ", 6) == 0)
		{
			if(check_str_valid(inbuf+6) != 0 || searchclient(inbuf+6) != NULL) send_priv_serv("Invalid name or name already in use,\nplease use only alphanumeric characters\n", connclient->userid);
			else
			{
				char oldname[11];
				strncpy(oldname, connclient->name, 10);
				oldname[10] = 0;
				memset(connclient->name, 0, 11);
				strncpy(connclient->name, (inbuf+6), 10);
				sprintf(outbuf, "%s changed name to %s\n",oldname, connclient->name);
				printf("%s", outbuf);
				send_all(outbuf);
			}
			continue;
		}
		else if(strncmp(inbuf, "/active", 7) == 0)
		{
			send_userlist(connclient);
			continue;
		}
		else if(strncmp(inbuf, "/channels", 9) == 0)
		{
			send_channellist(connclient);
			continue;
		}
		else if(strncmp(inbuf, "/join ", 6) == 0)
		{
			channel* joinc = find_channel(inbuf+6);
			if(joinc == NULL) send_priv_serv("Cannot find no especified channel.\n", connclient->userid);
			else
			{
				connclient->channel = joinc;
				send_priv_serv("Channel joined.\n", connclient->userid);
			}
			continue;
		}
		else if(strncmp(inbuf, "/create ", 8) == 0)
		{
			if(check_str_valid(inbuf+8) != 0 || find_channel(inbuf+8) != NULL) send_priv_serv("Invalid name or name already in use,\nplease use only alphanumeric characters\n", connclient->userid);
			else
			{
				char channelname[21];
				strncpy(channelname, (inbuf+8), 20);
				channelname[20] = 0;
				channel* new = init_channel(channelname);
				connclient->channel = new;
				send_priv_serv("Channel created and joined.\n", connclient->userid);
			}
			continue;
		}
		else if(strncmp(inbuf, "/w ", 3) == 0)
		{
			char receiver[21];
			char message[100];
			sscanf(inbuf, "/w %99[^ ] %99[^\n]", receiver, message);
			client* recv = searchclient(receiver);
			if(recv != NULL)
			{
				send_whisper(message, connclient->name, recv);
				continue;
			}
			send_priv_serv("Cannot find specified user!\n", connclient->userid);
			continue;
		}
		else if(strncmp(inbuf, "/", 1) == 0)
		{
			send_priv_serv("Invalid command!\n", connclient->userid);
			continue;
		}
		else
		{
			if(connclient->channel == NULL)
			{
				send_priv_serv("You have not joined a channel yet! Please join a channel or create a new channel", connclient->userid);
				
			}
			else
			{
				sprintf(outbuf, "%s:\t%s\n",connclient->name, inbuf);
				send_all_channel(outbuf, connclient->channel);
			}
		}
	}
	close(connclient->clientfd);
	printf("%s closed connection.\n", connclient->name);
	clientList_drop(connclient->userid);
	return NULL;
}

void* socket_initializer(int* sockfd)
{
	int acctfd;
	struct sockaddr_in6 clientaddr;
	
	while(1)
	{
		//pthread_t* pthp = calloc(1, sizeof(pthread_t));
		printf("listening for thread\n");
		socklen_t clientlen = sizeof(clientaddr);
		acctfd = accept(*sockfd, (struct sockaddr*)&clientaddr, &clientlen);
		client* newclient = clientList_add(acctfd, clientaddr);
		pthread_create(newclient->pthp, NULL, (void *)&connection_handler, newclient);
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
	chl.first = NULL;
	chl.last = NULL;
	struct sockaddr_in6 servaddr;
	pthread_t pts;
	char input[100];
	void* ret = NULL;
	welcome = load_welcome("welcome.txt");
	
	if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
	{
        	perror("socket error");
        	return 1;
    	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port = htons(port);
	servaddr.sin6_addr = in6addr_any;
	
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
	channelList_empty();
	close(sockfd);
	if(welcome) free(welcome);
	printf("Server closed.\n");
	return 0;
}
