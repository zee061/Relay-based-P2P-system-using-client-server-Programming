#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
using namespace std;

//to show error
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

//displaying output
void Display()
{
	cout << "-----------------------------------------------------------------------------------\n";
	cout << "Server IP\tPort\toperation\tprotocol\tMore Info..\n";
	cout << "-----------------------------------------------------------------------------------\n";
}

//connecting client to peer nodes and server
int connectpeer(char *address, int portno, char *filename)
{
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent * server;
	struct in_addr ipv4addr;
	char buffer[256];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		error("error opening socket");
	}
	inet_pton(AF_INET, address, &ipv4addr);
	server = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	//connect to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR connecting");
	}

	cout << "\t\t\t\t\t\t\tConnection to the Peer SUCCESSFUL.\n";
	char req[50];
	string buff = "REQUEST : FILE :";
	sprintf(req, "%s %s", buff.c_str(), filename);
	cout << address << "\t";
	cout << portno << "\t";
	cout << "REQUEST \t";
	cout << "tcp     \t";
	cout << "Request to the peerNode : " << req << endl;

	//requesting peer info from server
	if (write(sockfd, req, strlen(req)) < 0)
	{
		error("ERROR writing to socket");
	}

	//reading server response
	bzero(buffer, 256);	//clearing buffer
	if (read(sockfd, buffer, 14) < 0)
	{
		error("ERROR reading from socket");
	}
	cout << address << "\t";
	cout << portno << "\t";
	cout << "RESPONSE\t";
	cout << "tcp     \t";
	cout << "Response from peer : " << buffer << endl;

	if (strcmp(buffer, "File NOT FOUND") == 0)
	{
		cout << "\t\t\t\t\t\t\tClosing the connection gracefully since FILE NOT FOUND on this node...\n";
		if (shutdown(sockfd, 0) < 0)
		{
			error("ERROR closing the connection");
		}
	}
	else if (strcmp(buffer, "FILE FOUND    ") == 0)
	{
		bzero(buffer, 256);
		FILE *save = fopen("sample1.txt", "w");
		int words;
		int count = 0;
		if (read(sockfd, &words, sizeof(int)) < 0)
		{
			//read the file content the peer is sending
			error("ERROR reading words from socket");
		}

		//printing file content on terminal
		cout << "File content is : ";

		while (count != words)
		{
			read(sockfd, buffer, 256);
			fprintf(save, "%s ", buffer);
			cout << buffer << " ";
			count++;
		}
		cout << "\ngracefully closing the connection with the peer....\n";
		if (shutdown(sockfd, 0) < 0)
		{
			error("error in closing the connection");
		}
		fclose(save);
		return 0;
	}
	else
	{
		cout << "received unknown reply from the node\n";
	}
	return -1;
}
int getFile(int sockfd, char IP[], int portno)
{

	//request for active peer information
	string req = "REQUEST : peer info";
	char buffer[256];
	int n;

	// Send message to the server
	if (write(sockfd, req.c_str(), req.length()) < 0)
	{
		error("ERROR writing to socket");
	}

	//read server response
	bzero(buffer, 256);
	if (read(sockfd, buffer, 255) < 0)
	{
		error("ERROR reading from socket");
	}
	cout << IP << "\t";
	cout << portno << "\t";
	cout << "RESPONSE\t";
	cout << "tcp     \t";
	cout << "Response from the server : \n";
	cout << "Server has the following info:\n";
	cout << buffer;
	cout << "gracefully closing the connection with the relay server....\n";
	if (shutdown(sockfd, 0) < 0)
	{
		error("ERROR closing the connection");
	}
	//store the info in a file
	FILE *peers = fopen("peer.txt", "w");
	fprintf(peers, "%s", buffer);	//reading the peer info from the buffer that is comming from the relay server and storing it into peer.txt file
	fclose(peers);

	char file[50];
	printf("Enter the File name : ");
	scanf("%s", file);
	//process the response one peer at a time and try to fetch the file
	char peerName[INET_ADDRSTRLEN];
	int port, flag = 0;
	peers = fopen("peer.txt", "r");
	while (fscanf(peers, "%s %d", peerName, &port) != EOF)
	{
		cout << peerName << "\t";
		cout << port << "\t";
		cout << "connect  \t";
		cout << "tcp     \t";
		cout << "Connecting to the peerNode\n";
		if (connectpeer(peerName, port, file) < 0)
		{
			continue;
		}
		else
		{
			flag = 1;
			break;
		}	//successfult found the file on this node
	}
	fclose(peers);
	if (!flag)
	{
		printf("File not found on any node!\n");
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent * server;

	char buffer[256];

	if (argc < 3)
	{
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);

	// Creating a socket descriptor
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		error("error opening socket");
	}

	server = gethostbyname(argv[1]);
	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	//connecting to the server
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR connecting");
	}

	Display();
	cout << argv[1] << "\t";
	cout << portno << "\t";
	cout << "connect  \t";
	cout << "tcp     \t";
	cout << "connection initiated to relay server\n";

	string req = "REQUEST : client";

	// Send message to the server
	if (write(sockfd, req.c_str(), req.length()) < 0)
	{
		error("ERROR writing to socket");
	}

	//read server response
	bzero(buffer, 256);
	if (read(sockfd, buffer, 255) < 0)
	{
		error("ERROR reading from socket");
	}

	//start server if node accepted by relay
	if (buffer[19] == 'C')
	{
		cout << argv[1] << "\t";
		cout << portno << "\t";
		cout << "RESPONSE  \t";
		cout << "tcp     \t";
		cout << "Response from the server : CLIENT connected SUCESSFULLY \n";

		cout << argv[1] << "\t";
		cout << portno << "\t";
		cout << "REQUEST  \t";
		cout << "tcp     \t";
		cout << "Request to the server : peer Info \n";
		if (getFile(sockfd, argv[1], portno) < 0)
		{
			error("error getting the requested file from the peers");
		}
	}
	else
	{
		printf("Node not accepted by the relay server, try again..\n");
	}
	return 0;
}