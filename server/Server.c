/*
 * client.c
 *
 *  Created on: Nov 11, 2023
 *      Author: francesco
 */


#ifdef WIN32
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"
#include "calculator.h"


typedef struct sockaddr_in sockAddr_in;


void clearwinsock();
int generateSocket(size_t namespace, size_t style, size_t protocol);
sockAddr_in setAddres(char *address,int family, int port);
int bindSocket(int socket, sockAddr_in *serverAddress);
int listening(int sock, int queue_len);
int accepting(int welcomeSocket, sockAddr_in *clientAddress);
int sendMsg(int clientSocket, char *msg, int flags);
int recvMsg(int clientSocket, char *buffer, size_t buffer_size, int flags);
/*
 * Function that takes the buffer (string) and extracts the operand and the operator
 * Format axpected operand operator1 operetor2 ... operatorNUM_OPERANDS
 */
int parser(char *operand, double op[NUM_OPERANDS], char *buffer);


int main(int argc, char *argv[]) {
#ifdef WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 0;
	}
#endif

	// create welcome socket
	int serverSocket;
	if ((serverSocket = generateSocket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1 )  return -1;

	//set connection settings
	sockAddr_in serverAddress;
	serverAddress = setAddres("127.0.0.1", AF_INET, PROTO_PORT);

	//binding socket to the address
	if( bindSocket(serverSocket, &serverAddress)  == -1 ) return -1;

	// listen
	if(listening(serverSocket, QLEN) == -1) return -1;


	sockAddr_in clientAddress; // structure for the client address
	int clientSocket;      	   // socket descriptor for the client


	while (1)
	{
		puts("Waiting for a client to connect...");

		// accept new connection
		if( (clientSocket = accepting(serverSocket, &clientAddress)) == -1) return -1;

		printf("Handling client %s = %s:%d\n",( gethostbyaddr((char *)&(clientAddress.sin_addr.s_addr), (socklen_t)sizeof(clientAddress), AF_INET)->h_name), inet_ntoa(clientAddress.sin_addr), htons(clientAddress.sin_port));

		for(int n = 0; n != 1; )
		{
			//waits for a msg from the client
			if(recvMsg(clientSocket, buffer,  BUFFER_SIZE - 1, 0) == -1) break;

			double op[NUM_OPERANDS]; //operands needed by the calculator
			char operand; //operator needed by the calculator

			n = parser(&operand, op, buffer);
			if(n == 0)
			{
				double result = doOperation(op, operand);
				snprintf(buffer, BUFFER_SIZE - 1, "%.3lf", result);
			}

			if(n == 1)
				if(sendMsg(clientSocket, "end", 0) == -1) break;


			if(n == -1)
				snprintf(buffer, BUFFER_SIZE - 1, "%s", "Wrong input. Correct format: op operand1 operand2 ..." );


			if(sendMsg(clientSocket, buffer, 0) == -1) break;

			memset(buffer, 0, BUFFER_SIZE - 1);

		}
		closesocket(clientSocket);
		clearwinsock();



	}

} // main end







void clearwinsock() {
#ifdef WIN32
	WSACleanup();
#endif
}

int generateSocket(size_t namespace, size_t style, size_t protocol)
{
	int sock = socket(namespace, style, protocol);
	if(sock != -1) puts("socket creata con successo");
	else
	{
		puts("Errore, non è stato possibile creare la socket");
		clearwinsock();
	}
	return sock;
}

sockAddr_in setAddres(char *address,int family, int port)
{
	sockAddr_in sock;
	memset(&sock, 0, sizeof(sock));
	sock.sin_addr.s_addr = inet_addr(address);
	sock.sin_port = htons(port);
	sock.sin_family = family;
	return sock;
}

int listening(int sock, int queue_len)
{

	int r;
	if((r = listen(sock, queue_len))>=0) puts("Socket in ascolto...");
	else
	{
		puts("Errore listening fallito");
		closesocket(sock);
		clearwinsock();
	}
	return r;
}

int accepting(int welcomeSocket, sockAddr_in *clientAddress)
{
	int newSocket;
	socklen_t clientAddress_len = sizeof(*clientAddress);
	if((newSocket = accept(welcomeSocket, (struct sockaddr *) clientAddress, &clientAddress_len )) >= 0)
		puts("Nuova connessione accettata.");
	else
	{
		puts("accept failed");
		closesocket(newSocket);
		clearwinsock();
	}
	return newSocket;
}

int sendMsg(int clientSocket, char *msg, int flags)
{

	if (send(clientSocket, msg, strlen(msg), flags) != strlen(msg))
	{
		puts("send() sent a different number of bytes than expected");
		closesocket(clientSocket);
		clearwinsock();
		return -1;
	}
	return 0;
}

int recvMsg(int clientSocket, char *buffer, size_t buffer_size, int flags)
{
	memset(buffer, '\0', BUFFER_SIZE);
	if ((recv(clientSocket, buffer, buffer_size-1, flags)) <= 0) {
		puts("recv() failed or connection closed prematurely");
		closesocket(clientSocket);
		clearwinsock();
		return -1;
	}//puts("Messaggio ricevuto");
	return 0;
}

int bindSocket(int socket, sockAddr_in *serverAddress)
{
	int r;
	if((r = bind(socket, (struct sockaddr* )serverAddress, sizeof(sockAddr_in))) < 0)
	{
		puts("binding fallito");
		perror("bind");
		closesocket(socket);
		clearwinsock();
	}
	else puts("Socket bindata con successo");

	return r;
}

int parser(char *operand, double op[NUM_OPERANDS], char *buffer)
{
	//setto a 0 tutti gli operandi
	memset(op, 0, sizeof(int) * NUM_OPERANDS);

	switch(buffer[0])
	{
	case '+':
		*operand = '+';
		break;
	case '-':
		*operand = '-';
		break;
	case '*':
		*operand = '*';
		break;
	case '/':
		*operand = '/';
		break;
	case '=':
		memset(buffer, 0, BUFFER_SIZE - 1);
		return 1;
	default:
		memset(buffer, 0, BUFFER_SIZE - 1);
		return -1;
	}
	//move the pointer to the buffer to where i should find the first operand
	buffer+=2;
	//strtod converte la stringa in un double e restituisce un puntatore a dove termina il numero trovato
	//se becca una stringa che non è convertibile ritorna 0
	for(int i = 0; i < NUM_OPERANDS && buffer != NULL; ++i)	op[i] = strtod(buffer, &buffer);


	memset(buffer, 0, BUFFER_SIZE - 1);

	return 0;


}





