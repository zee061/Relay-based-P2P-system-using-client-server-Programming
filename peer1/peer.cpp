#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
using namespace std;

//to show error
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void Display()
{
	cout << "-----------------------------------------------------------------------------------\n";
	cout << "Server IP\tPort\toperation\tprotocol\tMore Info..\n";
	cout << "-----------------------------------------------------------------------------------\n";
}

int startserver(char *port)	//to start the server we only need port number
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		error("ERROR opening socket");
	}
	bzero((char*) &serv_addr, sizeof(serv_addr));
	portno = atoi(port);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	int check = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (check < 0)
	{
		error("ERROR on binding");
	}

	listen(sockfd, 5);	//max 5 active connections allowed

	cout << ".........\t";
	cout << portno << "\t";
	cout << "Listen   \t";
	cout << "tcp     \t";
	cout << "Server running on peer node, listening.....\n";

	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0)
	{
		error("ERROR on accept");
	}
	close(sockfd);
	bzero(buffer, 256);
	if (read(newsockfd, buffer, 255) < 0)
	{
		error("ERROR reading from socket");
	}

	//check if the message is a request for file
	char a[] = "REQUEST : FILE : ";
	int i, flag = 1;

	for (int i = 0; i < strlen(a); i++)
	{
		if (a[i] != buffer[i])
		{
			flag = 0;
			break;
		}
	}
	//if message is a file request
	if (flag)
	{
		cout << "peerClient\t";
		cout << ntohs(cli_addr.sin_port) << "\t";
		cout << "REQUEST \t";
		cout << "tcp     \t";
		cout << "Received request for the file : " << &buffer[strlen(a)] << " from the client\n";

		FILE *file = fopen(&buffer[strlen(a)], "r");	//opening file

		if (file == NULL)
		{
			cout << "\t\t\t\t\t\t\trequested file NOT Found\n";
			char response[] = "FILE NOT FOUND";

			if ((n = write(newsockfd, response, strlen(response))) < 0)
			{
				error("ERROR writing to socket");
			}
		}
		else
		{
			cout << "\t\t\t\t\t\t\tFound the requested file \n";
			char response[] = "FILE FOUND    ";
			if (write(newsockfd, response, strlen(response)) < 0)
			{
				error("ERROR writing to socket");
			}

			//sending file to client
			int words = 1;
			char c;
			char buff[256];
			bzero(buff, 256);
			while ((c = getc(file)) != EOF)
			{
				fscanf(file, "%s", buff);
				if (isspace(c) || c == '\t')
					words++;
			}
			//sending number of words to client
			if (write(newsockfd, &words, sizeof(int)) < 0)
			{
				error("ERROR writing words to socket");
			}
			rewind(file);

			char ch;
			while (ch != EOF)
			{
				fscanf(file, "%s", buff);
				write(newsockfd, buff, 256);	//sending each word to client
				ch = fgetc(file);
			}

			fclose(file);
		}
	}
	else
	{
		cout << "received request is not of file name,closing connection......\n";
		close(newsockfd);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent * server;	//hostent is a structure that comes under 
#include <netdb.h>

	char buffer[256];

	if (argc < 3)
	{
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);

	// Creating socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		error("ERROR opening socket");
	}

	server = gethostbyname(argv[1]);

	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	bzero((char*) &serv_addr, sizeof(serv_addr));	//clearing serv_addr
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);	//bcopy is used to copy the bytes from server to serv_addr
	serv_addr.sin_port = htons(portno);	//htons function is used to convert IP port number in host byte order to the IP port number in network byte order.

	//connect to the server
	int n1 = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (n1 < 0)
	{
		perror("ERROR connecting");
	}

	Display();
	cout << argv[1] << "\t";
	cout << portno << "\t";
	cout << "connect  \t";
	cout << "tcp     \t";
	cout << "connection initiated to server\n";

	string req = "REQUEST : node";

	//Sending request to the server
	if (write(sockfd, req.c_str(), req.length()) < 0)
	{
		perror("ERROR writing to socket");
	}

	//read The server response
	bzero(buffer, 256);
	if (read(sockfd, buffer, 255) < 0)
	{
		perror("ERROR reading from socket");
	}

	//start server if node accepted by relay
	if (buffer[17] == 'N')
	{
		cout << argv[1] << "\t";
		cout << portno << "\t";
		cout << "RESPONSE\t";
		cout << "tcp     \t";
		cout << "Response from the server : NODE CONNECTED SUCESSFULLY \n";

		if (shutdown(sockfd, 0) < 0)
		{
			error("ERROR closing the connection");
		}
		//start the server
		startserver(&buffer[20]);	//Now start the server with the port number stored at &buffer[20].
	}
	else
	{
		cout << "Node not accepted by the relay server\n";
	}
	return 0;
}