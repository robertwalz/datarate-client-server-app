#include <stdio.h>
#include <stdlib.h>  
#include <unistd.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include<sys/wait.h> 

#define PORT "7777"
#define BACKLOG 10
#define BLOCK_SIZE 102400

int main(int arvc, char* argv[]){

	int serverSocket, serverBinding, serverListening, newSocket;

	struct addrinfo hints, *res;
	struct sockaddr_storage clientAddr;
	socklen_t clientAddrLen;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // AF_INET or AF_INET6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, PORT, &hints, &res);

	/* Create a socket */
	if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	/* for testing */
	int optval = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &optval, sizeof(optval));
	
	/* Bind socket to port */
	if((serverBinding = bind(serverSocket, res->ai_addr, res->ai_addrlen)) == -1){
		perror("bind");
		exit(EXIT_FAILURE);
	}

	/* Listen on port */
	if((serverListening = listen(serverSocket, BACKLOG)) == -1){
		perror("listen");
		exit(EXIT_FAILURE);
	}

	printf("Server is waiting for incoming connections on port " PORT " ...\n");

	while(1){

		/* Waiting for connections */

		newSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);

		if(newSocket < 0){
			perror("new socket");
			close(serverSocket);
			close(newSocket);
			exit(EXIT_FAILURE);
		}
		
		struct sockaddr_in *addr_in = (struct sockaddr_in *)&clientAddr;
		char *ipStr = inet_ntoa(addr_in->sin_addr);
		printf("Create new task for request from %s\n", ipStr);
		/* Create child process for new connection */
		int process = fork();

		/* Check in which process child or parent */
		if(process < 0){
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if(process == 0){
			/* Child process */
		  pid_t pid = getpid();
			printf("In child process (PID: %d)\n", pid);
			
			char *blockBuffer[BLOCK_SIZE];		
			int blocksReceived = 0;

			while(read(newSocket, blockBuffer, BLOCK_SIZE)>0){
				blocksReceived++;
				int convertedNumber = htonl(blocksReceived);
				write(newSocket, &convertedNumber, sizeof(convertedNumber));
			}
			close(newSocket);
			printf("Received %d blocks from %s...\n", blocksReceived, ipStr);
			printf("Exiting child process (PID: %d)\n", pid);
			exit(0);
		 
			}
		else{ // > 0
			/* Parent process */
			signal(SIGCHLD,SIG_IGN); // prevents from zombie child processes ...
		  pid_t pid = getpid();
			printf("Parent process (PID: %d) is waiting for incomming connections...\n", pid);
			close(newSocket);
		}

	}


}
