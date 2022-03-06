#include <assert.h>
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
#include <sys/msg.h>
#include <arpa/inet.h>

#define MAX_PENDING 5
#define BUFFSIZE 256
#define DATABASE_LOC "database.txt"
// Trie will be indexed for index 0 to 9, for each digit
#define TRIE_CHILD_COUNT 10


typedef void Sigfunc(int);
key_t myKey;

typedef struct msgbuf
{
	long mtype;
	pid_t sender_pid;
	char text[64];
} msgbuf;

typedef struct TrieNode
{
	char* data;
	struct TrieNode* children[TRIE_CHILD_COUNT];
} TrieNode;

typedef struct Trie{
	TrieNode* root;
} Trie;

char** put(Trie* trie, int index)
{
	assert(index >= 0);

	if (trie->root == NULL)
		trie->root = calloc(1, sizeof(TrieNode));

	TrieNode* traverse = trie->root;

	while (index > 9)
	{
		int trieIndex = index % 10;
		index /= 10;

		if (!traverse->children[trieIndex])
			traverse->children[trieIndex] = calloc(1, sizeof(TrieNode));

		traverse = traverse->children[trieIndex];
	}

	if (!traverse->children[index])
		traverse->children[index] = calloc(1, sizeof(TrieNode));

	traverse = traverse->children[index];

	return &traverse->data;
}

char* get(Trie* trie, int index)
{	
	if (trie->root == NULL)
		return NULL;

	TrieNode* traverse = trie->root;

	if (index == 0)
	{
		if (traverse->children[0] == NULL)
			return NULL;
	
		return traverse->children[0]->data;
	}

	while (traverse && index)
	{
		traverse = traverse->children[index % 10];
		index /= 10;
	}

	if (!traverse)
		return NULL;

	return traverse->data;
}

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

void flush(FILE* fp, TrieNode* t, int val)
{
	if (t == NULL)
		return;
	
	if (t->data != NULL)
		fprintf(fp, "%d %s\n", val, t->data);
	
	for (int i = 0; i < TRIE_CHILD_COUNT; ++i)
		flush(fp, t->children[i], val * 10 + i);
}

void flushDatabaseToFile(Trie* t)
{
	FILE* fp = fopen(DATABASE_LOC, "w");
	
	if (t != NULL)
		flush(fp, t->root, 0);

	fclose(fp);
}

char* performOperation(int argc, char** argv, Trie* t)
{
	printf("Command to execute: ");

	for (int i = 0; i < argc; ++i)
		printf("%s ", argv[i]);
	printf("\n");

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

		flushDatabaseToFile(t);
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
		flushDatabaseToFile(t);
		return "OK";
	}

	// This must be good bye
	return "Goodbye";
}

void do_task(int connfd, struct sockaddr_in *cliaddr, socklen_t clilen)
{
	int qid = msgget(myKey, IPC_CREAT | 0660);

	if (qid == -1)
	{
		perror("qid error");
		exit(-1);
	}

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

		// prepare the message
		msgbuf msg;
		msg.sender_pid = getpid();
		msg.mtype = 1;
		memset(msg.text, 0, sizeof(msg.text));
		strcpy(msg.text, buff);

		// send the message to database
		int len = sizeof(msgbuf) - sizeof(long);
		int result = msgsnd(qid, &msg, len, 0);


		if (result == -1)
		{
			perror("child queue write error");
			exit(-1);
		}

		// read the reply of database
		result = msgrcv(qid, &msg, sizeof(msgbuf) - sizeof(long), getpid(), 0);

		if (result == -1)
		{
			perror("queue read error");
			exit(-1);
		}
		
		// send the response back to client
		n = write(connfd, msg.text, strlen(msg.text) + 1);
		if (n < 0)
		{
			perror("error sending response");
			exit(-1);
		}
		
		if (strcmp("Goodbye", msg.text) == 0)
			break;
	}
}

void databaseProcess()
{
	// Load database from file
	
	char* fileLoc = DATABASE_LOC;
	Trie t;
	t.root = NULL;

	// read database from file, if it exists
	if (access(fileLoc, F_OK) == 0)
	{
		// read file
		FILE* fp = fopen(fileLoc, "r");
		int val;
		char buff[BUFFSIZE];

		while (fscanf(fp, "%d %s", &val, buff) != EOF)
		{
			char** ptr = put(&t, val);
			*ptr = calloc(strlen(buff) + 1, sizeof(char));
			strcpy(*ptr, buff);
		}
	}

	// get attached to queue
	int qid = msgget(myKey, IPC_CREAT | 0660);

	if (qid == -1)
	{
		perror("queue create error");
		exit(-1);
	}


	while (1)
	{
		int result, length;
		msgbuf buffer;

		length = sizeof(msgbuf) - sizeof(long);

		// get the request
		if ((result = msgrcv(qid, &buffer, length, 1, 0)) == -1)
		{
			perror("queue reading error");
			exit(-1);
		}

		// process the request
		char* buff = buffer.text;
		int n = strlen(buff);

		// -- split the request
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

		// get the response from database
		char* res = performOperation(argCount, args, &t);

		// send the response back to sender
		buffer.mtype = buffer.sender_pid;
		buffer.sender_pid = 0;
		strcpy(buffer.text, res);


		result = msgsnd(qid, &buffer, sizeof(msgbuf) - sizeof(long), 0);
		if (result == -1)
		{
			perror("queue write error");
			exit(-1);
		}
	}
}

void main(int argc, char** argv)
{
	myKey = ftok(".", 'a');

	if (fork() == 0)
	{
		databaseProcess();
		exit(0);
	}

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
