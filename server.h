#ifndef QUEUE_H
#define QUEUE_H


typedef struct client_t client;
typedef struct clientlist clientList;

struct client_t
{
	char name[20];
	int userid;
	struct sockaddr_in addr;
	int clientfd;
	client* next;
};

struct clientlist
{
    client* first;
    client* last;
};

client* clientList_add(int clientfd, struct sockaddr_in clientaddr);

void clientList_drop(int id);

void send_whisper(char* msg, int send_uid, int recv_uid);

void send_priv_serv(char* msg, int uid);

void send_all(char* msg);

void* connection_handler(client* connclient);

void* socket_initializer(int* sockfd);



#endif
