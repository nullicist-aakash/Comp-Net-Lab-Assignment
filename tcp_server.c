#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include "Trie.h"

#define MAX_PENDING 5
#define BUFFSIZE 256

typedef void Sigfunc(int);
Trie t;

Sigfunc* Signal(int signo, Sigfunc* func)
{
	struct sigaction act, oldact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (sigaction(signo, &act, &oldact) < 0)
		return SIG_ERR;

	return oldact.sa_handler;
}

void sig_child(int signo)
{
	pid_t pid;
	int stat;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("Child terminated: %d\n", pid);
}

char* performOperation(int argc, char** argv, Trie* t)
{
/*	printf("Received: ");

	for (int i = 0; i < argc; ++i)
		printf("'%s' ", argv[i]);
	printf("\n");
*/
	char* reqErr = "Invalid Request";

	if (argc < 1 || argc > 3)
		return reqErr;
	if (argc == 1 && strcmp("Bye", argv[0]) != 0)
		return reqErr;
	if (argc == 2 && strcmp("get", argv[0]) != 0 && strcmp("del", argv[0]) != 0)
			return reqErr;

	if (argc == 3 && strcmp("put", argv[0]))
		return reqErr;


	// put operation
	if (strcmp(argv[0], "put") == 0)
	{
		int key = atoi(argv[1]);
		char* val = argv[2];

		char** storedVal = put(t, key);

		if (*storedVal != NULL)
			return "Key already exists";
		
		*storedVal = calloc(strlen(val), sizeof(char));
		strcpy(*storedVal, val);
		return "OK";
	}

	// get operation
	if (strcmp(argv[0], "get") == 0)
	{
		int key = atoi(argv[1]);
		char* storedVal = get(t, key);

		if (storedVal == NULL)
			return "Key not found";

		return storedVal;
	}

	// delete operation
	if (strcmp(argv[0], "del") == 0)
	{
		int key = atoi(argv[1]);
		char** storedVal = put(t, key);
		if (*storedVal == NULL)
			return "Key not found";
		free(*storedVal);
		*storedVal = NULL;
		return "OK";
	}

	// This must be good bye
	return "Goodbye";
}

void do_task(int connfd, struct sockaddr_in *cliaddr, socklen_t clilen)
{
	int n;
	char buff[BUFFSIZE];
	
	printf("Connection established with client: %s\n", inet_ntoa(cliaddr->sin_addr));

	while (1)
	{
		// read the input from client
		n = read(connfd, buff, BUFFSIZE);
		
		if (n <= 0)
		{
			perror("read error");
			exit(-1);
		}
		buff[n] = 0;

		// split the input on the basis of spaces

		int argCount = 1;
		for (int i = 0; i < n; ++i)
			if (buff[i] == ' ')
				++argCount;

		char** args = calloc(argCount, sizeof(char*));
		argCount = 1;
		args[0] = buff;

		for (int i = 0; i < n; ++i)
		{
			if (buff[i] == ' ')
			{
				args[argCount++] = &buff[i] + 1;
				buff[i] = '\0';
			}
		}

		char* res = performOperation(argCount, args, &t);
		
		n = write(connfd, res, strlen(res) + 1);
		if (n < 0)
		{
			perror("error sending response");
			exit(-1);
		}
		
		if (strcmp("Goodbye", res) == 0)
			break;
	}
}

void main(int argc, char** argv)
{
	t.root = NULL;
	key_t myKey = ftok(".", 'a');

	int listenfd;
	struct sockaddr_in servaddr;

	if (argc != 2)
	{
		printf("usage: server.o <PORT number>\n");
		exit(1);
	}

	// create the CLOSED state
	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenfd < 0)
	{
		perror("socket error");
		exit(1);
	}

	// bind the server to PORT
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("binding error");
		exit(1);
	}
	

	printf("Listening to PORT %d...\n", ntohs(servaddr.sin_port));

	// move from CLOSED to LISTEN state, create passive socket
	if (listen(listenfd, 5) < 0)
	{
		perror("listen error");
		exit(1);
	}

	// Attach the signal handler
	
	Signal(SIGCHLD, sig_child);

	// wait for connections
	while (1)
	{
		struct sockaddr_in cliaddr;
		socklen_t clilen = sizeof(cliaddr);
		int connfd;
		pid_t childpid;

		// do three way handshake
		if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) < 0)
			if (errno == EINTR)
				continue;
			else
				perror("accept error");


		// fork and process
		if ((childpid = fork()) == 0)
		{
			close(listenfd);	
			do_task(connfd, &cliaddr, sizeof(clilen));
			
			exit(0);
		}

		close(connfd);
	}
}
