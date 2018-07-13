/*******************************************************************************
* Name: COMPE 560 socket programming assignment 3
* File: server_multi.c <jason>
* Author: Hrishikesh Adigal
* email: hadigal@sdsu.edu
* Usage: ./server.out <portno>
* Date: 04/28/2018
*******************************************************************************/

//including some standard libraries to required for sockets
#include <sys/types.h> //UNIX socket library
#include <sys/socket.h> //UNIX socket library
#include <netinet/in.h> //IP addrs(sock addr)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFF 500

char ret_null = '\0';

// client address info
struct client_info
{
	int sockno; //socket descriptor
	char ip[INET_ADDRSTRLEN];
	int portno;
};

// global var to hold the max 10 clients
int clients[10];
// count
int n = 0;

// thread synchronization initializer
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//function to reverse the string msg from the connected clients
char *rev_str(char * msg,char * rev_buff, char *cli_ip,int portno)
{
	int len = 0;
	int msg_len = strlen(msg);
  //clearing the char recv_buff of any garbage value and setting all value to 0
  memset(rev_buff,0,BUFF);
  //***** string reverse *****
  int i,j;
  //reversing the data received from the client!!
  for(i = 0,j = msg_len - 1; i < msg_len; i++,j--)
  {
     rev_buff[i] = msg[j];
  }
	rev_buff[msg_len] = '\0';
  printf("\n************** Received string from client [ip:%s port:%d] **************\n%s\n",cli_ip,portno,msg);
  printf("\n************** Reversed string **************\n%s\n",rev_buff);
	return rev_buff;
}

// send function for handling multiple client connections
void sendtoclient(char *msg,int curr, char *ip, int portno)
{
	int i, ret, msg_len = strlen(msg);
	char * rev_buff = (char *)malloc(msg_len*sizeof(char));
	// using mutex for synchronization
	pthread_mutex_lock(&mutex);
	rev_buff = rev_str(msg,rev_buff,ip, portno);
	// sending the reversed string of the message from the connected client
	ret = send(curr,rev_buff,strlen(rev_buff),0);
	// send() error handling
	if(ret < 0)
	{
		perror("sending failure");
		exit(1);
	}
	// release the resources for the gicen thread
	pthread_mutex_unlock(&mutex);
}

// thread function for saving the data received from client and updating the client list
void *recvmg(void *sock)
{
	// address info on the client
	struct client_info cl = *((struct client_info *)sock);
	// msg buffer
	char * msg;

	msg = (char *)malloc(BUFF*sizeof(char));
	if(!msg)
	{
		printf("error in malloc(msg)!!\n");
	}

	int len,i,j;
	// receive data from the incoming client
	len = recv(cl.sockno,msg,BUFF,0);
	while(len > 0)
	{
		msg[len] = '\0';
		sendtoclient(msg,cl.sockno,cl.ip,cl.portno);
		memset(msg,'\0',sizeof(msg));
		len = recv(cl.sockno,msg,BUFF,0); // keep receiving data from the incoming clients
	}
	pthread_mutex_lock(&mutex);
	printf("\n************** Current connection status **************\nClient IP:%s disconnected\n",cl.ip);

	// updating the client connection list
	for(i = 0; i < n; i++)
	{
		if(clients[i] == cl.sockno)
		{
			j = i;
			while(j < n-1)
			{
				// updating the clients socket descriptor array
				clients[j] = clients[j+1];
				j++;
			}
		}
	}
	//decrement the client count after disconnection
	n--;
	//close(cl.sockno);
	pthread_mutex_unlock(&mutex);
}

int main(int argc,char *argv[])
{
	// address structure
	struct sockaddr_in srv_addr,cli_addr;
	//resetting all the values of the structure serv_addr
  bzero(&srv_addr, sizeof(srv_addr));
	//sock descriptors
	int sd;
	int nsd;
	// client addr len type
	socklen_t cli_addr_len;
	//port no.
	int portno;
	int ret;
	int cli_port;

	//send and receive operation thread
	pthread_t sendt,recvt;

	int len;
	// get client information
	struct client_info cl;
	char ip[INET_ADDRSTRLEN];

	// input error handling
	if(argc > 2)
	{
		printf("too many arguments. Help:\n%s <port>\n",argv[0]);
		exit(1);
	}
	else if(argc < 2)
	{
		printf("too few arguments. Help:\n%s <port>\n",argv[0]);
		exit(1);
	}

	// Server information
	portno = atoi(argv[1]);
	// creating server socket
	sd = socket(AF_INET,SOCK_STREAM,0);

	//memset(srv_addr.sin_zero,'\0',sizeof(srv_addr.sin_zero));
	srv_addr.sin_family = AF_INET; // connection family type => UNIX TCP/IP connection
	srv_addr.sin_port = htons(portno); // port number
	srv_addr.sin_addr.s_addr = INADDR_ANY; //IP address defined in struct in_addr.
	cli_addr_len = sizeof(cli_addr); // client addrenss length
	// performing bind on the server socket created
	ret = bind(sd,(struct sockaddr *)&srv_addr,sizeof(srv_addr));
	if(ret != 0)
	{
		perror("binding unsuccessful");
		exit(1);
	}
	printf("bind() successfull...\n");
	//Server creates a Queue to accept pending requests. Here max. 5 incoming requests will to given server connection
	ret = listen(sd,5);
	if(ret != 0) {
		perror("listening unsuccessful");
		exit(1);
	}
	printf("create a list of client Queue using listen() successfully...\n");
	// accept all incoming client connections
	while(1)
	{
		// accpeting incoming clients connection request
		nsd = accept(sd,(struct sockaddr *)&cli_addr,&cli_addr_len);
		if(nsd < 0)
		{
			perror("accept unsuccessful");
			exit(1);
		}
		printf("accept() successful...\n");
		// thread synchronization
		pthread_mutex_lock(&mutex);
		// convert binary to text the IPv4 address
		inet_ntop(AF_INET, (struct sockaddr *)&cli_addr, ip, INET_ADDRSTRLEN);
		char *temp = inet_ntoa(cli_addr.sin_addr);
		strcpy(ip,temp);
		cli_port = ntohs(cli_addr.sin_port);
		printf("\n************** incoming connection **************\nClient:%d IP:%s PORT:%d connected\n",n+1,ip,cli_port);
		// initializing the client struct
		cl.sockno = nsd;
		strcpy(cl.ip,ip);
		cl.portno = cli_port;
		clients[n] = nsd; // updating the clients socket descriptor array
		n++; // updating the clients count
		// Creating  the receive thread to rx message from the client individually
		int th_ret = pthread_create(&recvt,NULL,recvmg,&cl);
		if(th_ret != 0)
		{
			perror("Error in thread!!");
			exit(-8);
		}
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}
