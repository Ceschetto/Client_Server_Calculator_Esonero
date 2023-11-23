/*
 * client.c
 *
 *  Created on: Nov 11, 2023
 *      Author: francesco
 */


#if defined WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif



#include <string.h>
#include <stdio.h>

#include "protocol.h"


typedef struct sockaddr_in sockAddr_in;


void clearwinsock();
int generateSocket(size_t namespace, size_t style, size_t protocol);
sockAddr_in setAddres(char *address, size_t family ,size_t port);
int connecting(int clientSocket, sockAddr_in *serverAddress, size_t serverAddress_len);
int recvMsg(int clientSocket, char *buffer, size_t buffer_size, int flags);
int sendMsg(int clientSocket, char *msg, int flags);
void flushStdin();

int main(int argc, char *argv[]) {
#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 0;
	}
#endif




	// create client socket
	int clientSocket;
	if((clientSocket = generateSocket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1 ) return -1;

	// set connection settings
	struct sockaddr_in serverAddress;
	serverAddress = setAddres("127.0.0.1", AF_INET, PROTO_PORT);

	// connection
	if (connecting(clientSocket, &serverAddress, sizeof(serverAddress)) == -1) return -1;


	// receive from server
	char buffer[BUFFER_SIZE];


	do
	{

		memset(buffer, 0 , BUFFER_SIZE);
		printf("Input for calculator: ");


		fgets(buffer, BUFFER_SIZE, stdin);
		/*
		 * We search in the buffer if there is \n. if we found it we overwrite to \0
		 * else we flush the stream stdin until we find the \n.
		 */
		char *return_finder;
		if((return_finder = strchr(buffer, '\n')) != NULL) *return_finder = '\0';
		else flushStdin();


		//sends the buffer to the server
		if(sendMsg(clientSocket, buffer, 0) == -1) break;

		//recive the buffer from the server
		if(recvMsg(clientSocket, buffer,  BUFFER_SIZE - 1, 0) == -1) break;

		//prints the buffer
		puts(buffer);

	}while(strcmp(buffer, "end") != 0);//the client goes untill the server sends the end message


	closesocket(clientSocket);
	clearwinsock();
	return 0;
} // main end




int generateSocket(size_t namespace, size_t style, size_t  protocol)
{
	int sock = socket(namespace, style, protocol);
	if(sock != -1) puts("socket created!");
	else
	{
		puts("socket() failed.");
		clearwinsock();
	}
	return sock;
}

sockAddr_in setAddres(char *address, size_t family, size_t port)
{
	sockAddr_in AddrSettings;
	memset(&AddrSettings, 0, sizeof(AddrSettings));
	AddrSettings.sin_addr.s_addr = inet_addr(address);
	AddrSettings.sin_port = htons(port);
	AddrSettings.sin_family = family;
	return AddrSettings;
}

int connecting(int clientSocket, sockAddr_in *serverAddress, size_t serverAddress_len)
{

	if (( connect(clientSocket, (struct sockaddr*) serverAddress, serverAddress_len)) < 0) {
		puts("Failed to connect.");
		perror("connect");
		closesocket(clientSocket);
		clearwinsock();
		return -1;
	}else puts("Connection established with the server");

	return 0;
}

int recvMsg(int clientSocket, char *buffer, size_t buffer_size, int flags)
{
	memset(buffer, '\0', BUFFER_SIZE);
	if ((recv(clientSocket, buffer, buffer_size-1, flags)) <= 0) {
		puts("recv() failed or connection closed prematurely");
		return -1;
	}
	return 0;
}

int sendMsg(int clientSocket, char *msg, int flags)
{

	if (send(clientSocket, msg, strlen(msg), flags) != strlen(msg))
	{
		puts("send() sent a different number of bytes than expected");
		return -1;
	}
	return 0;
}

void clearwinsock()
{
#if defined WIN32
	WSACleanup();
#endif
}

void flushStdin()
{
	while(getchar() != '\n');
}



