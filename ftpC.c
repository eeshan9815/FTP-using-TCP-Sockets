/*    THE CLIENT PROCESS */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
int delimit(char a)
{
	char delims[] = {',', '.', ';', ':', ' ', '\t', '\n'};
	int i;
	for(i=0;i<7;i++)
	{
		if(a == delims[i])
			return 1;
	}
	return 0;
}


int main()
{
	int			sockfd;
	int			sockfd1;
	struct sockaddr_in	serv_addr;
	struct sockaddr_in	serv_addr1;

	int i;
	char buf[100];
	char command[80];
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    		perror("setsockopt(SO_REUSEADDR) failed");

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(50000);


	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	while(1) {
		for(i=0; i < 80; i++) command[i] = '\0';
		printf("> ");
		gets(command);
		send(sockfd, command, strlen(command) + 1, 0);
		int data_port;
		if(strncmp(command, "get ", 4) == 0) {

			// if(recv(sockfd, buf, 80, 0) == -1) {
			//
			// }
			pid_t pid = fork();
			//Cd child
			char filename[100];
			strcpy(filename, command + 4);
			if(pid == 0) {
				sleep(1);
				if ((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					perror("Unable to create socket\n");
					exit(0);
				}
				if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
							perror("setsockopt(SO_REUSEADDR) failed");

				serv_addr1.sin_family	= AF_INET;
				inet_aton("127.0.0.1", &serv_addr1.sin_addr);
				serv_addr1.sin_port	= htons(data_port);


				if ((connect(sockfd1, (struct sockaddr *) &serv_addr1,
									sizeof(serv_addr1))) < 0) {
					perror("Unable to connect to server1\n");
					exit(0);
				}

				int bytes_read = 0;
				int fd = open(filename, O_CREAT | O_WRONLY, 0666);
				if(fd < 0) {
					printf("Cannot write into client file\n");
					// return 1;
				}

				int n = recv(sockfd1, buf, 100, 0);
				if(n == 0) {
					printf("File not found\n");
					// return 2;
				}


				int words = 0;
				int s = 1;
				words += 1;
				while(n) {
					char header = buf[0];
					int bytes = (int)(buf[1]) * 256 + (int)(buf[2]);
					// buf += 3;
					if (n == 0 || n == -1)
						break;
					bytes_read += n - 3;
					int i;
					for (i = 3; i < n; ++i)
					{
						if(!s) {
							if(!delimit(buf[i])) {
								s = !s;
								words++;
							}
						}
						else {
							if(delimit(buf[i])) {
								s = !s;
							}
						}
					}
					int n1 = write(fd, buf + 3, n - 3);
					if(n1 != -1) {
						for(i=0; i < 100; i++) buf[i] = '\0';
						n = recv(sockfd1, buf, 100, 0);
					}
					else {
						printf("Failed to write into file\n");
						break;
					}
					if(header == 'L') {
						printf("File transfer was successful.\nSize of the file: %d\nNumber of words: %d\n", bytes_read, words);
						break;
					}
				}
				if(n == -1) {
					printf("Receive failed\n");
				}
				close(fd);
				close(sockfd1);
			}
			//Cc parent
			else {
				// int r = recv(sockfd, buf, 80, 0);
				// if(r == -1) { //or error code = 550
				// 	printf("Cd killed by Cc\n");
				// 	kill(pid, SIGKILL);
				// }
				short code;
				int r = recv(sockfd, &code, sizeof(code), 0);
				code = ntohs(code);
				printf("Code: %d\n", code);
				if(r==-1 || code==550)
				{
					printf("File could not be opened\n");
					kill(pid, SIGKILL);
					// kill(ret, SIGTERM);
				}

				//TODO accept return code and handle
			}
		}

		else if(strncmp(command, "put ", 4)==0)
		{
			int ret = fork();
			if(ret==0)			//in child
			{
				int sockfd1;
				struct sockaddr_in cli_addr1, serv_addr1;

				if((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				{
					perror("TCP Server socket creation failed for D");
					exit(EXIT_FAILURE);
				}
				if (setsockopt(sockfd1, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
			    	perror("setsockopt(SO_REUSEADDR) failed for D");


				serv_addr1.sin_family		= AF_INET;
				serv_addr1.sin_addr.s_addr	= INADDR_ANY;
				serv_addr1.sin_port			= htons(data_port);

				if(bind(sockfd1, (struct sockaddr *) &serv_addr1, sizeof(serv_addr1)) < 0)
				{
					perror("TCP Server bind failed for D");
					// close(sockfdD);
					exit(EXIT_FAILURE);
				}

				listen(sockfd1, 5);
				int clilen1 = sizeof(cli_addr1);
				int newsockfd1 = accept(sockfd1, (struct sockaddr *) &cli_addr1, &clilen1);
				if(newsockfd1<0)
				{
					perror("Accept error in D");
					close(sockfd1);
					exit(EXIT_FAILURE);
				}

				char fileData[100];
				// char *token = strtok(buf, " \t");
				int fd = open(command+4, O_RDONLY, 0666);
				if(fd<0)
				{
					// perror("No file found on client");
					close(newsockfd1);
					exit(EXIT_FAILURE);
				}

				char dataFromFile[100];
		    	short bytes = 1;
		    	while(bytes)
		    	{
		    		bytes = read(fd, dataFromFile+3, 97);
		    		if(bytes<0) break;
		    		if(bytes==97)
		    			dataFromFile[0]='X';
		    		else dataFromFile[0]='L';		//last block
					// printf("%s", dataFromFile+3);

	    			dataFromFile[1]=bytes/256;
	    			dataFromFile[2]=bytes%256;

			    	int t = send(newsockfd1, dataFromFile, bytes+3, 0);
			    	if(t<0) {
			    		printf("Send failed\n");
			    	}
			    	memset(&dataFromFile, '\0', sizeof(dataFromFile));
		    	}

		    	// printf("File %s sent successfully\n", buf+4);


				close(sockfd1);
				close(fd);
				exit(EXIT_SUCCESS);
			}
			else
			{
				// printf("in parent\n");
				// send(sockfd, buf, strlen(buf)+1, 0);
				short code;
				// printf("here\n");
				recv(sockfd, &code, sizeof(code), 0);
				// printf("here?\n");
				code = ntohs(code);
				printf("Code: %d\n", code);

				if(code==550)
				{
					printf("File could not be opened\n");
					// kill(ret, SIGTERM);
				}

			}
		}
		else if(strncmp(command, "port ", 5) == 0) {
			data_port = atoi(command + 5);
			short code;
			recv(sockfd, &code, sizeof(code), 0);
			code = ntohs(code);
printf("Code: %d\n", code);
			if(code == 550 || code == 503) {
				close(sockfd);
				break;
			}

		}
		else {
			short code;
			recv(sockfd, &code, sizeof(code), 0);
			code = ntohs(code);
printf("Code: %d\n", code);
			if(code == 421) {
				close(sockfd);
				break;
			}
			if(code == 550 || code == 503) {
				close(sockfd);
				break;
			}

		}

	}
	/*
	for(i=0; i < 100; i++) buf[i] = '\0';
	recv(sockfd, buf, 100, 0);
	printf("%s\n", buf);


	strcpy(buf,"Message from client");
	send(sockfd, buf, strlen(buf) + 1, 0);

	close(sockfd);
	*/
}
