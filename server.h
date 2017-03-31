#ifndef CHATSERVER_H
#define CHATSERVER_H


typedef struct client_t client;
typedef struct clientlist clientList;
typedef struct channel_t channel;
typedef struct channellist channellList;

struct client_t
{
	char name[20];
	int userid;
	struct sockaddr_in addr;
	int clientfd;
	channel* channel;
	pthread_t* pthp;
	client* next;
};

struct clientlist
{
	client* first;
	client* last;
};

struct channel_t
{
	char name[21];
	unsigned int users;
	client** clients;
	client** admins;
	channel* next;
};

struct channellist
{
	channel* first;
	channel* last;
};

char* load_welcome(const char* filename);

channel* find_channel(const char* name);

channel* init_channel(const char* name);

int join_channel(const char* chn, client* cl);

int leave_channel(client* cl);

client* clientList_add(int clientfd, struct sockaddr_in clientaddr);

void clientList_drop(int id);

void clientList_empty();

void send_whisper(char* msg, int send_uid, int recv_uid);

void send_userlist(client* connclient);

void send_priv_serv(char* msg, int uid);

void send_all(char* msg);

void* connection_handler(client* connclient);

void* socket_initializer(int* sockfd);



#endif
