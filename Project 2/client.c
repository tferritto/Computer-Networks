// This is the client application that will be used to send requests to the server.
// It takes 3 arguments, server IP or host, port, and the file requested.

// List of includes
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> // for close
#include <string.h>

#define MAXBUF 1024


int main (int argc,char* argv[]) {

  char cServer[20];
  char cPort[6];
  char cFile [200];
  int nPort;
  
  if (argc < 4)
    {
      printf ("Usage: client server_name server_port file_requested \r\n");
      return(0);
    }
  
  sprintf(cServer,"%s",argv[1]);

  sprintf(cPort,"%s",argv[2]);
  nPort = atoi(cPort);

  sprintf(cFile,"%s",argv[3]);
  
  struct sockaddr_in name;
  struct hostent *hent;
  int sd;
  // create a new socket to use for communication
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("(cliConn): socket() error");
    exit (-1);
  }
  // Used to correctly fill in the server host
  if ((hent = gethostbyname (cServer)) == NULL)
    fprintf (stderr, "Host not found");
  else
    bcopy (hent->h_addr, &name.sin_addr, hent->h_length);
 
  name.sin_family = AF_INET;
  name.sin_port = htons (nPort); // notice the host to network conversion.
 
  // connect to the server
  if (connect (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("(cliConn): connect() error");
    exit (-1);
  }
  // send the request to the server, the request is simply the file name passed to the client from command line
  write (sd, cFile, sizeof(cFile));
  
  char buffer[MAXBUF];
  int bytes_read = 0;
  int total_bytes_read = 0;
  // keep reading data from the server, until done.
  do {
    memset(buffer,0,MAXBUF);
    bytes_read = read(sd, buffer, MAXBUF-1);
    if (bytes_read < 0)
       break;
    fprintf(stdout,"Read %d bytes in buffer\r\n");
    total_bytes_read += bytes_read;
  }
  while ( bytes_read > 0 );
  
  printf("Total Bytes Read = %d\r\n",total_bytes_read);
  close(sd);

  return 0;
}
