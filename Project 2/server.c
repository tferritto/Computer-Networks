#include <sys/time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include <unistd.h>

#include "log.h" //thread safe log(str,...) macro.

#define MAX_BUFFER_SIZE 1024 //based on buffer size in the client.c
#define MAX_CLIENTS 64
#define logFileName "log.txt"

//FILE *logFile;
pthread_mutex_t lock;
pthread_t thread;
int sd;

void* fileTransfer(void* arg);

void shutDownServer();

int main(int argc, char* argv[])
{
	signal(SIGINT, shutDownServer);
    
	if(argc != 2)
	{
		printf("Error: Invalid argument count.");
		return -1;
	}
    
	logFile = fopen(logFileName,"a"); //open/create logfile to be appended to.
	fprintf(logFile,"\n----------------------------------------\n");
	pthread_mutex_init(&lock,NULL);
    
	const int server_port = atoi(argv[1]);
	//int sd;
	struct sockaddr_in sockIn;
	sockIn.sin_family = AF_INET;
	sockIn.sin_port = htons(server_port);
	sockIn.sin_addr.s_addr = INADDR_ANY;
    
	log("Starting server on port %d.\n", server_port);
    
	//Creating a new socket.
	if( (sd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		log("Failed to create socket.\n", NULL);
		shutDownServer();
		exit(-1);
	}
    
	if( bind(sd, (struct sockaddr*) &sockIn, sizeof(sockIn)) == -1)
	{
		log("Failed to bind.\n", NULL);
		shutDownServer();
		exit(-1);
	}
    
	if( listen(sd,MAX_CLIENTS) == -1 )
	{
		log("Failed to listen.\n", NULL);
		shutDownServer();
		exit(-1);
	}
    
	struct sockaddr_in tmpConnection;
	int tmpLen = sizeof(struct sockaddr_in);
	int tmpSocket;
    
	int threadCount = 0;
	while(1)
	{
		tmpSocket = accept(sd,(struct sockaddr*)&tmpConnection, &tmpLen);
        
		if(tmpSocket == -1)
		{
			log("Failed to accept to client socket.\n",NULL);
		}
		pthread_create(&thread, NULL, (void*)&fileTransfer, (void*)tmpSocket);
		threadCount++;
	}
    
	shutDownServer();
	return 0;
}

void* fileTransfer(void* arg)
{
    
	int clientId = (int)arg; //client socket
	pthread_t threadID = pthread_self();
	log("Thrd:%lu - Connected to a client.\n",(unsigned long)threadID);
    
	char file[256]; //file name that the client is requesting
	char* fileBuffer[MAX_BUFFER_SIZE];
	memset(fileBuffer,0,MAX_BUFFER_SIZE);
	int filePointer;
	int fileSize;
	int buff;
	struct stat fileInfo;
    
	struct timeval startTime;
	struct timeval finishTime;
	double timeDifInSec;
	double transferSpeed;
    
	read(clientId, file, 256);
    
	if(stat(file,&fileInfo) == -1)
	{
        
		char* err;
		err = "FILE NOT FOUND";
		write(fcntl(clientId, F_SETFD, fcntl(clientId,F_GETFD) &~O_NONBLOCK),err,15);  /* SEE NOTE 1 IN READ ME */
		shutdown(clientId,SHUT_WR);
		close(clientId);
		log("Thrd:%lu - Requested file not found. File: %s .",(unsigned long)threadID,file);
        
		return NULL;
	}
    
	filePointer = open(file,O_RDONLY);
    
    
	log("Thrd:%lu - Starting transfer of file: %s - %d bytes.\n",(unsigned long)threadID, file, (int)fileInfo.st_size);
    
	gettimeofday(&startTime,NULL);
	log("Thrd:%Lu - Sleeping 5 seconds.\n",(unsigned long)threadID);
	sleep(5);
	int bytesRead = 0;
	int sent;
	while( bytesRead = read(filePointer,fileBuffer,MAX_BUFFER_SIZE) )
	{
		sent = write(clientId,fileBuffer,bytesRead);
		memset(fileBuffer,0,MAX_BUFFER_SIZE);
		log("Thrd:%lu - BytesRead: %d\n",(unsigned long)threadID, bytesRead);
		log("Thrd:%lu - BytesSent: %d\n",(unsigned long)threadID, sent);
	}
	shutdown(clientId,SHUT_WR);
	gettimeofday(&finishTime,NULL);
    
	timeDifInSec = (finishTime.tv_sec + finishTime.tv_usec/1000000) - (startTime.tv_sec + startTime.tv_usec/1000000);
	transferSpeed = fileInfo.st_size / timeDifInSec;
    
	log("Thrd:%lu - Finished transfering file to client.\n",(unsigned long)threadID);
	log("Thrd:%lu - \tTotal time: %f sec.\n",(unsigned long)threadID, timeDifInSec);
	log("Thrd:%lu - \tTransfer rate: %f Bps.\n",(unsigned long)threadID, transferSpeed);
    
	close(filePointer);
	close(clientId);
    
	return NULL;
}

void shutDownServer()
{
	log("Shutting down server.\n",NULL);
	close(sd);
	fclose(logFile);
	pthread_mutex_destroy(&lock);
	pthread_exit(&thread);
}
