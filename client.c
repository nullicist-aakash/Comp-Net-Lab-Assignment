#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define MAX_PENDING 5
#define BUFFSIZE 32

void do_task(int sockfd)
{
	int n;
	char buff[BUFFSIZE];

	while (1)
	{
		printf("Enter request: ");
		scanf("%[^\n]%*c", buff);

		n = write(sockfd, buff, strlen(buff) + 1);
		if (n < 0)
		{
			perror("write error!");
			exit(-1);
		}

		n = read(sockfd, buff, BUFFSIZE);
		if (n <= 0)
		{
			perror("receive error");
			exit(1);
		}
		buff[n] = '\0';

		printf("Server replied: %s\n", buff);

		if (strcmp(buff, "Goodbye") == 0)
			break;
	}
}

void main(int argc, char** argv)
{
	int sockfd;
	struct sockaddr_in servaddr;

	if (argc != 3)
	{
		printf("usage: client.o <IPaddress> <PORT>\n");
		exit(1);
	}

	// create socket in CLOSED state
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	// fill the server details
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

	// perform three way handshake
	if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect error");
		exit(1);
	}

	do_task(sockfd);
	close(sockfd);

	return;
}
