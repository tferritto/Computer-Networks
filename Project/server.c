#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

pthread_mutex_t lock;
pthread_t srvthread;
FILE *logfile;
int serversocket;

void * server(void * attributes);
void killserver();

int main(int argc, char *argv[]){
    
    // Application Startup:
    // socket port [logfilefilename]
    // if name is not specified, assume logfile.txt in local directory
    
    signal(SIGINT,killserver);
    
    char *logfilename;
    int portnum;
    
    if (argc < 2){
        fprintf(stderr,"Incorrect format:\n");
        fprintf(stderr,"server portnum [log file]\n");
        exit(EXIT_FAILURE);
    }
    
    // Assign Arguments to values
    portnum = atoi(argv[1]);
    if (argc > 2)
        logfilename = argv[2];
    else
        logfilename = "logfile.txt";
    
    // Create Log File.  If the file does not exist, create it.  If it does
    // exit, then append to it.
    logfile = fopen(logfilename,"a");
    
    // Check to see if logfile file can be opened
    if (logfile == NULL){
        fprintf(stderr,"Unable to open/create Log File.  Terminating...\n");
        exit(EXIT_FAILURE);
    }
    
    // Print out Program Preamble
    fprintf(stdout,"Multithreaded Socket File Server\n");
    fprintf(stdout,"Logging status to %s (appending if the file exists)\n",logfilename);
    fprintf(stdout,"Press Ctrl-C (SIGINT) to gracefully kill server\n");
    
    // Start Logging information using Time Information
    struct timeval time1;
    struct tm *timeinfo;
    gettimeofday(&time1,NULL);
    timeinfo = localtime(&time1.tv_sec);
    fprintf(logfile,"========\n");
    fprintf(logfile,"Starting Server %s",asctime(timeinfo));
    fflush(logfile);
    
    // Using fprintf for logfileging as fprintf is a thread-safe function.  Note that once we
    // are in a thread, we should use a mutex to prevent interleaving of the logfile entries.
    
    // Setup Socket Connection
    serversocket = socket(AF_INET,SOCK_STREAM,0);
    
    // Check if Socket Opened
    if (serversocket < 0){
        gettimeofday(&time1,NULL);
        timeinfo = localtime(&time1.tv_sec);
        fprintf(stderr,"Unable to Open Socket... Terminating...\n");
        fprintf(logfile,"%d:%02d:%02d %d - Unable to Open Socket.  Exiting...",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec);
        fclose(logfile);
        exit(EXIT_FAILURE);
    }
    
    // Setting Socket Parameters
    struct sockaddr_in serveraddress;
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_addr.s_addr = INADDR_ANY;
    serveraddress.sin_port = htons(portnum);
    
    // Bind to Socket
    if (bind(serversocket,(struct sockaddr *) &serveraddress, sizeof(struct sockaddr_in))){
        fprintf(stderr,"Unable to Bind Socket... Terminating...\n");
        gettimeofday(&time1,NULL);
        timeinfo = localtime(&time1.tv_sec);
        fprintf(logfile,"%d:%02d:%02d %d - Unable to Bind Socket.  Exiting...",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec);
        fclose(logfile);
        exit(EXIT_FAILURE);
    }
    
    // Make socket Listen with a maximum of 30 clients
    if (listen(serversocket,30) < 0){
        fprintf(stderr,"Unable to Listen on Socket... Terminating...\n");
        gettimeofday(&time1,NULL);
        timeinfo = localtime(&time1.tv_sec);
        fprintf(logfile,"%d:%02d:%02d %d - Unable to Listen on Socket.  Exiting...",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec);
        fclose(logfile);
        exit(EXIT_FAILURE);
    }
    
    // Start Accepting Connection Requests
    struct sockaddr_in clientcon;
    int clientsocket;
    
    // Setup Thread Locking for Log Files
    if (pthread_mutex_init(&lock,NULL) != 0){
        fprintf(stderr,"Unable to Set Locking... Terminating...\n");
        gettimeofday(&time1,NULL);
        timeinfo = localtime(&time1.tv_sec);
        fprintf(logfile,"%d:%02d:%02d %d - Unable to Set Locking.  Exiting...",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec);
        fclose(logfile);
        exit(EXIT_FAILURE);
    }
    
    // Logging Time for Server to Start Listening for connection
    gettimeofday(&time1,NULL);
    timeinfo = localtime(&time1.tv_sec);
    fprintf(logfile,"%d:%02d:%02d %d - Waiting for Connections...\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec);
    fflush(logfile);
    
    // Accepting Connection and forking off to a thread
    while(1){
        
        // Accept a connection.  Hang until connection is made.
        socklen_t addr_length = sizeof(struct sockaddr_in);
        clientsocket = accept(serversocket,(struct sockaddr *)&clientcon,(socklen_t*)&addr_length);
        if (clientsocket < 0){
            fprintf(stderr,"Unable to Open Client Socket...\n");
            gettimeofday(&time1,NULL);
            timeinfo = localtime(&time1.tv_sec);
            fprintf(logfile,"%d:%02d:%02d %d - Unable to Open Client Socket...\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec);
            break;
        }
        
        // Spawn a thread and call the server() function to handle file transfers.  Log if the thread was not able to be created.
        int threadattributes = clientsocket;
        if (pthread_create(&srvthread,NULL,(void *)&server,(void *)threadattributes) !=0){
            fprintf(stderr,"Unable to Create Thread...\n");
            gettimeofday(&time1,NULL);
            timeinfo = localtime(&time1.tv_sec);
            fprintf(logfile,"%d:%02d:%02d %d - Unable to Create Thread...\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec);
            break;
        }
        
    } // End While Loop
    
    // Tear Down program.
    killserver();
    return EXIT_SUCCESS;
}


// killserver() will terminate the socket server by closing the server socket and the log file, and disposing of the mutex
// that allows the thread to write to the log file in a thread safe manner.  Finally, it destroys the threading engine.
void killserver(){
    struct timeval time1;
    struct tm *timeinfo;
    fprintf(stdout,"Shutting Down Server...\n");
    gettimeofday(&time1,NULL);
    timeinfo = localtime(&time1.tv_sec);
    fprintf(logfile,"%d:%02d:%02d %d - Shutting down server...\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec);
    fclose(logfile);
    close(serversocket);
    pthread_mutex_destroy(&lock);
    pthread_exit(&srvthread);
}


// server() will handle the connection and file transfer aspects of the program.  It uses MUTEX's in order to protect writing to the log file.
void * server(void * attributes){
    
    // Setup timing variables for logging and rate calculations.
    struct timeval time1;
    struct timeval time2;
    struct tm *timeinfo;
    
    // Reinitialize Variables
    int localclientid;
    
    // Initialize Variables passed to the thread
    char * filename;
    pthread_t threadid = pthread_self();
    localclientid = attributes;
    
    // Get Filename from client
    filename = malloc (128 * sizeof(char)); //Set maximum file+path to 128.
    read(localclientid,filename,128);
    
    // Log Connection
    pthread_mutex_lock(&lock);
    fprintf(stdout,"Connection Request on Thread %lu\n",(unsigned long)threadid);
    gettimeofday(&time1,NULL);
    timeinfo = localtime(&time1.tv_sec);
    fprintf(logfile,"%d:%02d:%02d %d - Thread %lu - Connection Established\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec,(unsigned long)threadid);
    pthread_mutex_unlock(&lock);
    
    // Check validity of filename.  If not, close connection and return to listen
    struct stat fileinfo;
    if (stat(filename,&fileinfo) < 0){
        char * status;
        status = "FILE NOT FOUND";
        write(localclientid,status,15);
        pthread_mutex_lock(&lock);
        gettimeofday(&time1,NULL);
        timeinfo = localtime(&time1.tv_sec);
        fprintf(stderr,"File Not Found - %s\b",filename);
        fprintf(logfile,"%d:%02d:%02d %d - Thread %lu - Filename Not Found - %s\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec,(unsigned long)threadid,filename);
        fflush(logfile);
        pthread_mutex_unlock(&lock);
        close(localclientid);
        return NULL;
    }
    
    // Log Request for File and track the start time of the file transfer.
    pthread_mutex_lock(&lock);
    gettimeofday(&time1,NULL);
    timeinfo = localtime(&time1.tv_sec);
    long int time1startsec = time1.tv_sec;
    long int time1startusec = time1.tv_usec;
    fprintf(logfile,"%d:%02d:%02d %d - Thread %lu - Request for filename %s\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time1.tv_usec,(unsigned long)threadid,filename);
    fflush(logfile);
    pthread_mutex_unlock(&lock);
    
    // Read File and Send out to Client
    int filedesc;
    filedesc = open(filename,O_RDONLY);
    int overallsize = fileinfo.st_size;
    
    
    // This section writes to the socket byte by byte.  It works 100% of the time, but is extremely slow
    /*int buffer;
     
     int size = 0;
     
     while(read(filedesc,&buffer,1) > 0){
     write(localclientid,&buffer,1);
     size++;
     }*/
    
    // This code is more efficient than what is above, but caused many problems because the socket was closed prior
    // to completing the send of data to the client.
    char * buffer = malloc (1024*sizeof(char));
    int readbytes;
    int size = 0;
    int sizesent = 0;
    int sentbytes;
    
    memset(buffer,0,1024);
    while ((readbytes=read(filedesc,buffer,1023)) >= 1){
        sentbytes = write(localclientid,buffer,readbytes);
        size += readbytes;
        sizesent += sentbytes;
        memset(buffer,0,1024);
    }
    
    // Ensure all data was sent.
    shutdown(localclientid,SHUT_WR);
    while (read(localclientid,buffer,1)>0); // Wait until the read returns 0, then close down the socket connection
    
    // Log Request for File
    pthread_mutex_lock(&lock);
    gettimeofday(&time2,NULL);
    timeinfo = localtime(&time2.tv_sec);
    long int time1endsec = time2.tv_sec;
    long int time1endusec = time2.tv_usec;
    
    // Calculate Time Differential and Rate Calculations
    
    double timestart = (double)time1startsec + ((double)(time1startusec)/1000000);
    double timeend = (double)time1endsec + ((double)(time1endusec)/1000000);
    double timediff = timeend - timestart;
    double rateinbps = (double)size*8/timediff;
    
    // Print Success Information and shut down this thread
    fprintf(stdout,"File %s successfully sent on Thread %lu (%i bytes/%i byes) (%f secs - %f bps)\n\n",filename,(unsigned long)threadid,(int)overallsize,(int)sizesent,timediff,rateinbps);
    fprintf(logfile,"%d:%02d:%02d %d - Thread %lu - %s sent successfully (%i bytes) (%f secs - %f bps)\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,(int)time2.tv_usec,(unsigned long)threadid,filename,(int)overallsize,timediff,rateinbps);
    fflush(logfile);
    pthread_mutex_unlock(&lock);
    close(filedesc);
    close(localclientid);
    return NULL;
}