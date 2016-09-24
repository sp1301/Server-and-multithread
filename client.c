#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include "port.h"

// error msg for errno
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int stage;
int sockfd; 

// this has one thread that goes on forever intill the user types exit and will always try to write to the server with the user input
void* SendInput(){
  int n = 0;
  char temp[5];
  temp[4] = '\0';
  char buffer[256];
  while(strcmp(temp, "exit") != 0){ //Receive messages and send input to the server in loop
    if(stage == 0){
      printf("Would you like to \"open <account name>\" an account or \"start <account name>\" an account or \"exit\"?\n");
    }
    else if(stage == 1){
      printf("Would you like to \"credit <amount>\" or \"debit <amount>\" or \"balance\" or  \"finish\"?\n");
    }

    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0){
      error("ERROR writing to socket");
	}
    strncpy(temp, buffer, 4);
    if(strcmp(temp, "exit") == 0){ // if the write is "exit" then end thread
      stage = -1; // this is to stop the receiveing thread
	  close(sockfd);
      pthread_exit(0);
    }
    sleep(2);
  }
  pthread_exit(0);
}



//this has one thread that goes on forever intill stage = -1. this happens in the sendinput thread. 
void* ReceiveOutput(){
  int n = 0;
  char temp[5];
  temp[4] = '\0';
  char buffer[256];
  while(stage != -1){ // this will happen when the user types "exit".
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
      error("ERROR reading from socket");
    if(strcmp(buffer, "Bank account has been made\n") == 0 || strcmp(buffer, "Bank successfully started\n") == 0)
      stage = 1;
    else if(strcmp(buffer, "Customer session ended\n") == 0)
      stage = 0;
    printf("%s\n",buffer);
  }
  close(sockfd);
  pthread_exit(0);
}

//this is where we make the client try to connect to the server using a port number of our choosing. The port number cna be changed on the port.h file
//also this is where we create 2 threads for the read and write. 
int main(int argc, char *argv[])
{
  // int sockfd, portno, n;
  int portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
	
    if (argc < 2) {
       fprintf(stderr,"usage %s hostname\n", argv[0]);
       exit(0);
    }
	
	
	//this is al for connection to server
    portno = serverport;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    while(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){ //failed connection - sleep for 3 seconds and attempt to reconnect
      printf("Failed connection, reattempting in 3 seconds\n");
      sleep(3);
    }
    
    write(sockfd, "connection has been made", 24); //after connection - write to server success
    bzero(buffer,256);
    n = read(sockfd,buffer,255); //receive success from server
    if (n < 0)
      error("ERROR reading from socket");
    printf("%s\n",buffer);
    
	
	// this is for the creation of the 2 threads for read and write.
	
    //create some arguments to pass to the threads
    // ... note they are malloced because they must be on the heap,
    //     so that they are visible to the threads
    int * threadArgs0 = (int*)malloc(sizeof(int));
    int * threadArgs1 = (int*)malloc(sizeof(int));
    *threadArgs0 = 0;
    *threadArgs1 = 1;

    // build thread status variables for pthread_exit to use later
    void* threadStatus0;
    void* threadStatus1;

    // build thread handles for pthread_create
    pthread_t thread0;
    pthread_t thread1;

    // build blank pthread attribute structs and initialize them
    pthread_attr_t threadAttr0;
    pthread_attr_t threadAttr1;
    pthread_attr_init(&threadAttr0);
    pthread_attr_init(&threadAttr1);

    // set the initialized attribute struct so that the pthreads created will be joinable
    pthread_attr_setdetachstate(&threadAttr0, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setdetachstate(&threadAttr1, PTHREAD_CREATE_JOINABLE);
    
    // set the initialized attribute struct so that the pthreads created will be kernel threads
    pthread_attr_setscope(&threadAttr0, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&threadAttr1, PTHREAD_SCOPE_SYSTEM);
    /*
      pthread_attr_setdetachstate(&threadAttr0, PTHREAD_CREATE_DETACHED);
      pthread_attr_setdetachstate(&threadAttr1, PTHREAD_CREATE_DETACHED);
    */
    // build the pthreads
    pthread_create(&thread0, &threadAttr0, SendInput, (void*)NULL);
    pthread_create(&thread1, &threadAttr1, ReceiveOutput, (void*)NULL);
    // destroy the pthread attribute structs, we're done creating the threads,
    //   we don't need them anymore
    pthread_attr_destroy(&threadAttr0);
    pthread_attr_destroy(&threadAttr1);
    
    // wait for the threads to finish .. make the threadStatus variables point to 
    //    the value of pthread_exit() called in each
    pthread_join( thread0, &threadStatus0);
    pthread_join( thread1, &threadStatus1);
	
    return 0;
}
// we are DONEEE!