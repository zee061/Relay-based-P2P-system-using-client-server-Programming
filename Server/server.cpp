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

//showing error
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

FILE * output;
int count = 0;

//storing peer node info
int storeNodeInfo(int port, int sock, char clntName[])
{
	int n;

	output = fopen("node_info.txt", "a+");	// append mode 
	fprintf(output, "%s %d\n", clntName, port);
	fclose(output);

	string resp = "RESPONSE : Node: N";
	char comma = ',';
	char buffer[50];
	sprintf(buffer, "%s%c %d", resp.c_str(), comma, port);	//it returns the total number of characters written excluding null-character 					     			appended in the buffer*/
	cout << "Server    \t";
	cout << port << "\t";
	cout << "RESPONSE\t";
	cout << "tcp     \t";
	cout << "sending " << resp << endl;

	if (write(sock, buffer, strlen(buffer)) < 0)
	{
		error("error writing to socket");
		return 0;
	}
	return 1;
}

//handling client requests
void clientHandler(int sock, char buffer[], int port)
{
	int n;
	string resp = "RESPONSE : client: C";
	cout << "Server    \t";
	cout << port << "\t";
	cout << "RESPONSE\t";
	cout << "tcp     \t";
	cout << "sending " << resp << endl;
	if (write(sock, resp.c_str(), resp.length()) < 0)
	{
		error("error writing");
	}

	if (read(sock, buffer, 255) < 0)
	{
		error("ERROR reading from socket");
	}
	cout << "peerClient\t";
	cout << port << "\t";
	cout << "REQUEST\t";
	cout << "tcp     \t";
	cout << "Request from the client - " << buffer << "\n";
	if (strcmp(buffer, "REQUEST : peer info") == 0)
	{
		//read the file that contains the peer info
		FILE *f = fopen("node_info.txt", "rb");	//open the file in binary format.

		/*SEEK_END : It denotes end of the file.
		  SEEK_SET : It denotes starting of the file.
		  SEEK_CUR : It denotes file pointer’s current position.
		*/
		fseek(f, 0, SEEK_END);	//move the file pointer to the end
		long fsize = ftell(f);	//tell the file pointer position w.r.t. starting of the file (i.e. size of the file)
		fseek(f, 0, SEEK_SET);	//send the pointer to beginning of the file
		//above three lines is use to find the size of the file.					

		char *str1 = (char*) malloc(fsize + 1);
		fread(str1, fsize, 1, f);
		fclose(f);

		str1[fsize] = 0;
		printf("Server has the following info:\n%s", str1);

		//send the info to client
		if (write(sock, str1, strlen(str1)) < 0)
		{
			//writing to the client
			error("ERROR writing to socket");
		}
	}
}

void Display()
{
	cout << "-----------------------------------------------------------------------------------\n";
	cout << "Server IP\tPort\toperation\tprotocol\tMore Info..\n";
	cout << "-----------------------------------------------------------------------------------\n";
}

int main(int argc, char *argv[])
{

	if (argc < 2)
	{
		fprintf(stderr, "usage %s port\n", argv[0]);
		exit(0);
	}
	output = fopen("node_info.txt", "w");
	fclose(output);

	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];

	struct sockaddr_in serv_addr, cli_addr;
	int n, pid;

	//socket creation
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		error("error opening socket");
	}

	//Initialize socket structure
	bzero((char*) &serv_addr, sizeof(serv_addr));	//use to clear the data on server
	portno = atoi(argv[1]);	//convert the string into integer

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);	//host to network short 

	//bind the host address using bind() call to server socket
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR on binding");
	}

	//start listening for the clients

	listen(sockfd, 5);	//5 is the maximum limit of the client that can connect at a time to the server.
	clilen = sizeof(cli_addr);
	cout << "Server started, now listening....to port number " << portno << endl;
	Display();

	//connecting client ot the server.
	while (1)
	{
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		count++;
		if (newsockfd < 0)
		{
			error("error on accepting");
		}

		//Create child process
		pid = fork();

		if (pid < 0)
		{
			error("ERROR on fork");
		}

		if (pid == 0)
		{
			//This is the client process
			close(sockfd);

			int n, flag = 0, port = ntohs(cli_addr.sin_port) + 100, sock = newsockfd;
			char buffer[256];
			bzero(buffer, 256);
			if (read(sock, buffer, 255) < 0)
			{
				//read the REQUEST message into the buffer comming from client.
				error("ERROR reading from socket");
			}
			//checking whether request is comming from the client or peer node.
			if (strcmp(buffer, "REQUEST : node") == 0)
			{
				flag = 1;
			}
			else if (strcmp(buffer, "REQUEST : client") == 0)
			{
				flag = 2;
			}

			if (flag == 1)
			{
				cout << "peerNode" << count << "\t";
				cout << port << "\t";
				cout << "Connect\t";
				cout << "tcp     \t";
				cout << "Received Message - " << buffer << "\n";

				//Store the IP address and port number of the peer node
				char clntName[INET_ADDRSTRLEN];

				//inet_ntop is used to convert network address into character string.
				if (inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL)
				{
					if (storeNodeInfo(port, sock, clntName) == 1)
					{
					 			//printf(" A peer is connected\n");					
					}
				}
				else
				{
					cout << "Unable to get address\n";
				}
			}
			else if (flag == 2)
			{

				//serving clients
				cout << "Client" << "\t";
				cout << port << "\t";
				cout << "Connect\t";
				cout << "tcp     \t";
				cout << "Received Message - " << buffer << "\n";
				clientHandler(sock, buffer, port);
			}
			else
			{
				cout << "ERROR : Unknown REQEST message, no action taken\n";
			}
			exit(0);
		}
		else
		{
			close(newsockfd);
		}
	}	// end of while
}