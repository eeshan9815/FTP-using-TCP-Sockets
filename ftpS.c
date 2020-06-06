/*
			NETWORK PROGRAMMING WITH SOCKETS

In this program we illustrate the use of Berkeley sockets for interprocess
communication across the network. We show the communication between a server
process and a client process.


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* The following three files must be included for network programming */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
			/* THE SERVER PROCESS */

int main()
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			sockfd1, newsockfd1 ; /* Socket descriptors */
	int			clilen;
	int			clilen1;
	struct sockaddr_in	cli_addr, serv_addr;
	struct sockaddr_in	cli_addr1, serv_addr1;
	char command[80];
	int i;
	char buf[100];
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    		perror("setsockopt(SO_REUSEADDR) failed");

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(50000);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);
	while(1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}
		int data_port;
		int first_command = 1;
		while(1) {
			recv(newsockfd, command, 80, 0);
			//printf("Received %s from client\n", command);
			if(first_command) {
				if(strncmp(command, "port ", 5) == 0) {
					data_port = atoi(command + 5);
					if(data_port < 1024 || data_port > 65535) {
						//TODO send error code 550
						short un = htons(550);
						printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
						close(newsockfd);
						break;
					}
					else {
						printf("Port number for data transfer will be %d \n", data_port);
						first_command = 0;
						//TODO send reply code 200
						short un = htons(200);
						printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
					}
				}
				else {
					//TODO send error code 503
					short un = htons(503);
					printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
					close(newsockfd);
					break;
				}
			}
			else {
				char args[100];
				if(strncmp(command, "cd ", 3) == 0) {
					for(i=0; i < 100; i++) args[i] = '\0';
					strcpy(args, command + 3);
					int t = chdir(args);
					if(t!=0) {
						short un = htons(501);
						printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
						continue;
					}
					char path[1000];
					printf("Current working directory is set to %s\n", getcwd(path, sizeof(path)));
					fflush(stdout);
					short un = htons(200);
					printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
				}

				else if(strncmp(command, "get ", 4) == 0) {
					// printf("get ittttttttt\n");
					strcpy(args, command + 4);
					printf("Opening file: %s\n", args);
					int fd = open(args, O_RDONLY);
					if(fd < 0) {
						printf("File not found\n");
						//TODO send error code 550
						short un = htons(550);
						printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
						printf("sending 550 get no file exist\n");
						continue;
						// close(newsockfd);
						// return 0;
					}
					// send(newsockfd, buf, 0, 0);
					pid_t pid = fork();
					//Sd child
					if(pid == 0) {
						char buf[100];
						if ((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
							perror("Cannot create socket\n");
							exit(0);
						}
						if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
									perror("setsockopt(SO_REUSEADDR) failed");

						serv_addr1.sin_family		= AF_INET;
						serv_addr1.sin_addr.s_addr	= INADDR_ANY;
						serv_addr1.sin_port		= htons(data_port);

						if (bind(sockfd1, (struct sockaddr *) &serv_addr1,
										sizeof(serv_addr1)) < 0) {
							perror("Unable to bind local address\n");
							exit(0);
						}
						printf("Server is listening\n");
						listen(sockfd1, 5);
						clilen1 = sizeof(cli_addr1);
						newsockfd1 = accept(sockfd1, (struct sockaddr *) &cli_addr1,
									&clilen1) ;

						if (newsockfd1 < 0) {
							perror("Accept error\n");
							exit(0);
						}
						int n = 97, n1;
						int exit_status = 0;
						int bytes_sent = 0;
						while(1) {
							n = read(fd, buf+3, 100-3);
							buf[0] = (n==97)?'X':'L';
							buf[1] = n/256;
							buf[2] = n%256;
							if(n>0) {
								bytes_sent += (n);
								n1 = send(newsockfd1, buf, n+3, 0);
								if(n1 != -1) {
									memset(&buf, '\0', sizeof(buf));
								}
								else{
									printf("Couldn't send\n");
									exit_status = -1;
									break;
								}
							}
							else {
								exit_status = 0;
								short un = htons(250);
								printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
								break;
							}
						}
						close(newsockfd1);
						exit(exit_status);
					}
					//Sc parent
					else {
						int status;
						waitpid(pid, &status, 0);
						status = WEXITSTATUS(status);
						if(status==0) {
							//TODO send code 250
// 							short un = htons(250);
// 							printf("Sending code: %d\n", ntohs(un));
// send(newsockfd, &un, sizeof(short), 0);
						}
						else if(status==-1) {
							//TODO send code 550
// 							short un = htons(550);
// 							printf("Sending code: %d\n", ntohs(un));
// send(newsockfd, &un, sizeof(short), 0);
						}
					}
				}

				else if(strncmp(command, "put ", 4) == 0) {
					strcpy(args, command + 4);
					// token = strtok(NULL, " \t");
					// printf("putssssss %s\n", token);

					pid_t retD = fork();
					if(retD==0)
					{
						int sockfd1;
						struct sockaddr_in	serv_addr1;
						sleep(1);
						if ((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
							perror("Unable to create socket for D\n");
							exit(EXIT_FAILURE);
						}

						serv_addr1.sin_family	= AF_INET;
						inet_aton("127.0.0.1", &serv_addr1.sin_addr);
						serv_addr1.sin_port	= htons(data_port);
						if ((connect(sockfd1, (struct sockaddr *) &serv_addr1,
											sizeof(serv_addr1))) < 0) {
							perror("Unable to connect to client D\n");
							// sleep(1);
							exit(EXIT_FAILURE);
						}

						// if(argcount!=2)
						// {
						// 	status = 501;
						// 	short un = htons(status);
						// 	printf("Sending code: %d\n", ntohs(un));
// send(newsockfd, &un, sizeof(short), 0);
						// 	continue;
						// }

						int fd = open(args, O_WRONLY | O_CREAT | O_TRUNC , 0666);

						if(fd<0) {
							short un = htons(550);
							printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
							continue;
						}
						printf("File %s opened on server\n", args);

						char fileData[100];

						int isLast=0;
						// printf("here\n");
						sleep(1);

						int bytes = recv(sockfd1, fileData, 100, 0);
						// printf("here, bytes: %d\n", bytes);
						if(bytes<=0)
						{
							printf("No file found on client\n");
							short un = htons(550);
							printf("Sending code: %d\n", ntohs(un));
							send(newsockfd, &un, sizeof(short), 0);
							close(fd);
							close(sockfd1);
							exit(EXIT_FAILURE);
						}
						short packet_size = fileData[1]*256 + fileData[2];
						// printf("file data :%s", fileData+3);
						int total = bytes;
						while(1)
						{
							if(fileData[0]=='L')
								isLast=1;
							packet_size = fileData[1]*256 + fileData[2];
							// printf("File desc: %d\n", fd);
							if(bytes<0)
							{
								perror("Recv failed");
								break;
							}

							if(write(fd, fileData+3, packet_size) < 0)
							{
								perror("Write failed");
								break;
							}
							if(isLast)
								break;
							memset(fileData, '\0', sizeof(fileData));
							bytes = recv(sockfd1, fileData, 100, 0);

							total+=packet_size;

						}
						printf("Size of file: %d\n", total);
						short un = htons(250);
						printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(un), 0);

						close(sockfd1);
						close(fd);
						exit(0);
					}
					else {

						// close(fd);
					}

				}
				else if(strcmp(command, "quit") == 0) {
					//TODO quit and close sockets
					short un = htons(421);
					printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
					close(newsockfd);
					break;
				}

				else {
					short un = htons(502);
					printf("Bad command: %s\n", command);
					printf("Sending code: %d\n", ntohs(un));
send(newsockfd, &un, sizeof(short), 0);
				}
			}
		}
		close(newsockfd);
	}
}
