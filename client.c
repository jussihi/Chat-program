#include <curses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

WINDOW *input, *output;
int connection_flag = 0;

void* message_listener(int* sockfd)
{
	char inbuf[1000];
	wrefresh(output);
	int n;
	int err = 0;
	while(1)
	{
		memset(inbuf, 0, 1000);
		n = read(*sockfd, inbuf, sizeof(inbuf));
		inbuf[n] = 0;
		if(n > 0)
		{
			wrefresh(output);
			waddstr(output, inbuf);
			wrefresh(output);
			wrefresh(input);
			err = 0;
		}
		else if(n == 0)
		{
			break;
		}
		else 
		{
			err++;
			if(err > 20)
			{
				connection_flag = -1;
				break;
			}
		}
	}
	return NULL;
}

void close_connection(pthread_t* ptmsg, int* sockfd)
{
	void* ret = NULL;
	if(ptmsg != NULL)
	{
		pthread_cancel(*ptmsg);
		pthread_join(*ptmsg, &ret);
	}
	if(*sockfd != 0) shutdown(*sockfd, 0);
	if(*sockfd != 0) close(*sockfd);
	connection_flag = 0;
	waddstr(output, "Disconnected...\n");
	wrefresh(output);
}

int main(void)
{
	
	/*INIT CURSES WINDOW*/
	char buffer[1000];
	initscr();
	cbreak();
	echo();
	input = newwin(1, COLS, LINES - 1, 0);
	output = newwin(LINES - 1, COLS, 0, 0);
	wmove(output, LINES - 2, 0);    /* start at the bottom */
	scrollok(output, TRUE);
	
	/*INIT SOCKET & CONNECTION*/
	int sockfd = 0;
	struct sockaddr_in servaddr;
	char address[100] = {0};
	int port;
	char portc[10];
	pthread_t ptmsg;
	void* ret = NULL;
	
	while (1)
	{
		mvwprintw(input, 0, 0, "> ");
		if(wgetnstr(input, buffer, COLS - 4) == OK)
		{
			if(strcmp(buffer, "/help") == 0)
			{
				waddch(output, '\n');
				waddstr(output, "Possible commands:\n\n/help\t\t\t\tShow this help screen\n/connect [server]:[ip]\t\tConnect to server. For example:\n/connect 127.0.0.1:5555\n/name [newname]\t\t\tSet new username\n/active\t\t\t\tShow active users on current server\n");
			}
			else if(strncmp(buffer, "/connect", 8) == 0)
			{
				if(connection_flag == 1)
				{
					waddstr(output, "ERROR: You are already connected to a server.\n");
				}
				else
				{
					if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
					{
						waddstr(output, "Socket error. Please try to restart program.\n");
						wrefresh(output);
						wgetnstr(input, buffer, COLS - 4);
						return 1;
					}
					sscanf(buffer, "/connect %99[^:]:%99d[^\n]", address, &port);
					sprintf(portc, "%d", port);
					waddstr(output, "\nConnecting to server ");
					waddstr(output, address);
					waddstr(output, " with port ");
					waddstr(output, portc);
					waddch(output, '\n');
					wrefresh(output);
					werase(input);
					memset(&servaddr, 0, sizeof(servaddr));
					servaddr.sin_family = AF_INET;
					servaddr.sin_port = htons(port);
					if(inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0)
					{
						waddstr(output, "inet_pton error\n");
						wrefresh(output);
						return 1;
					}
					if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
					{
						waddstr(output, "Connection error\n");
						wrefresh(output);
						return 1;
					}
					else if(connection_flag == 0)
					{
						wrefresh(output);
						pthread_create(&ptmsg, NULL, (void *)&message_listener, &sockfd);
						connection_flag = 1;
					}
				}
			}
			else if(strcmp(buffer, "/quit") == 0)
			{
				if(ptmsg != 0)
				{
					pthread_cancel(ptmsg);
					pthread_join(ptmsg, &ret);
				}
				if(sockfd != 0) close(sockfd);
				connection_flag = 0;
				endwin();
				return 0;
			}
			else if(strcmp(buffer, "/disconnect") == 0 || connection_flag == -1)
			{
				if(connection_flag == 0)
				{
					waddstr(output, "You are already disconnected!\n");
					wrefresh(output);
				}
				else
				{
					close_connection(&ptmsg, &sockfd);
				}
			}
			else if(connection_flag == 1)
			{
				write(sockfd, buffer, strlen(buffer));
			}
		}
		werase(input);
		wrefresh(output);
	}
	endwin();
	return 0;
}
