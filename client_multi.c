/*******************************************************************************
* Name: COMPE 560 socket programming assignment 3
* File: client_multi.c <volta>
* Author: Hrishikesh Adigal
* email: hadigal@sdsu.edu
* Usage: ./client.out <jason.sdsu.edu| [IP addr: jason]> <portno>
* Date: 04/28/2018
*******************************************************************************/

//including some standard libraries to required for sockets
#include <sys/types.h> //UNIX socket library
#include <sys/socket.h> //UNIX socket library
#include <netinet/in.h> // sock addr (IP addrs)
#include <arpa/inet.h>
#include<limits.h>
#include<netdb.h> //gethostbyname()
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// msg buff size
#define MAX_BUFF_SIZE 500

// thread function for receiving reversed data from client
void *recvmg(void *sock)
{
	//sock descriptor
	int sd = *((int *)sock);
	//msg buffer
	char * msg = (char *)malloc(MAX_BUFF_SIZE*sizeof(char));
	if(!msg)
	{
		printf("error in malloc(msg) in recvmsg()\n");
		exit(EXIT_FAILURE);
	}
	memset(msg,0,MAX_BUFF_SIZE);
	// length of the message
	int len;
	// receive msg from server
	len = recv(sd,msg,MAX_BUFF_SIZE,0);
	if(len == -1)
	{
		perror("ERROR in recv() for reversed sent message: ");
		exit(6);
	}
	printf("\n::::::::::: The reversed string of the sent msg is recevied from server JASON :::::::::::\n");
	// Using fwrite to write to stdout the reversed data received from the server jason
	fwrite(msg,sizeof(char),strlen(msg),stdout);
	printf("\n");
	fflush(stdout);
	if(msg)
	{
		free(msg);
	}
}

int main(int argc, char *argv[])
{
	// sockaddr structure
	struct sockaddr_in srv_addr;
	int csd;
	int ssd;
	int portno;

	//creating thread objects
	pthread_t sendt,recvt;

	// vars. for server socket
	char username[100];
	char ip[INET_ADDRSTRLEN];
	int len,ret,th_ret;

	// error check on user input
	if(argc > 3)
	{
		printf("too many arguments. Help:\n%s <server URL|ipaddr> <port>\n",argv[0]);
		exit(1);
	}
	else if(argc < 3)
	{
		printf("too few arguments. Help:\n%s <server URL|ipaddr> <port>\n",argv[0]);
		exit(1);
	}
	//getting portno and server address from user input
	portno = atoi(argv[2]);
	strcpy(username,argv[1]);

	//structure to get the ip addr[network info] for the given server addr
	struct hostent *host_addr;
	//double pointer for addr_list that we get from the hostent
	struct in_addr **s_ip;

	//char buff to hold the ip addr for the server
	char *serv_ip;
	//using gethostbyname() to get the handle to structure host_addr of type hostent
	host_addr = gethostbyname(username);
	if(host_addr == NULL)
	{
		perror("HOST_NAME error: ");
		exit(7);
	}
	s_ip = (struct in_addr **)host_addr->h_addr_list;

	// getting the server ip addr for the URL passed by the user
	// from the h_addr_list of addrs eg. you will get 130.191.166.3 for jason.sdsu.edu
	int count;
	for(count = 0; s_ip[count] != NULL; count++)
	{
		serv_ip = inet_ntoa(*s_ip[count]);
	}
	printf("\nServer:%s IP:%s\n",username,serv_ip);
	//create client socket
	csd = socket(AF_INET,SOCK_STREAM,0);
	memset(srv_addr.sin_zero,'\0',sizeof(srv_addr.sin_zero));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(portno);
	//srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	srv_addr.sin_addr.s_addr = inet_addr(serv_ip); // supports both localhost as well as ip address

	// making a new connection with Server
	ret = connect(csd,(struct sockaddr *)&srv_addr,sizeof(srv_addr));
	if(ret < 0)
	{
		perror("connection not esatablished");
		exit(1);
	}
	printf("connect() succcess...\n");
	// convert binary to text the IPv4 address
	inet_ntop(AF_INET, (struct sockaddr *)&srv_addr, ip, INET_ADDRSTRLEN);
	printf("connected to server [%s] IP addr.: %s\n",username,serv_ip);
	//creating thread to handle send request
	th_ret = pthread_create(&recvt,NULL,recvmg,&csd);
	if(th_ret != 0)
	{
		perror("Error in creating thread\n");
		exit(7);
	}

	// Taking user input from stdin and saying it in usr_ip buffer before sending to server jason
	char *usr_ip = (char *)malloc(MAX_BUFF_SIZE);
	int ip_len;
	// clearing the char buffer of any garbage value and setting all value to 0
	memset(usr_ip,0,MAX_BUFF_SIZE);
	printf("\n*********** Enter your msg for server ***********\n");
	// User can input data of any type via stdin and save it char buffer created
	int data_len = 0;
	while(1)
	{
		ip_len = fread(usr_ip,sizeof(char),MAX_BUFF_SIZE,stdin);
		printf("\nSTDOUT:\n");
		// printing the user input data to stdout
		fwrite(usr_ip,sizeof(char),MAX_BUFF_SIZE - data_len,stdout);
		// fflush(c_buff);
		fflush(stdout);
		data_len = strlen(usr_ip);
		// breaking out of the loop if there is a EOF interrupt from user
		if(ip_len < MAX_BUFF_SIZE)
		{
			if(feof(stdin) || data_len >= MAX_BUFF_SIZE)
			{
				break;
			}
		}
	}
	// fread error handling
	if(ip_len <= 0)
	{
		perror("ERROR in reading usr input msg: ");
		exit(3);
	}

	// send msg from user to server
	printf("\n***********************************************\n");
	// sending msg for user-client to server
	ret=send(csd,usr_ip,strlen(usr_ip),0);
	if (ret == -1)
	{
		perror("ERROR in SEND(): ");
		exit(4);
	}
	printf("send() succcess\n");
	// joining the socket
	pthread_join(recvt,NULL);
	// closing the socket descriptor
	if(usr_ip)
	{
		free(usr_ip);
	}
	close(csd);
}
