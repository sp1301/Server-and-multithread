#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <malloc.h>
#include "bank.h"
#include <signal.h>
#include "port.h"




struct LinkedList{ // to hold the threads and socket number of every client that connects to the server
  pthread_t thread;  
  int newsockfd;
  struct LinkedList* next;
};
typedef struct LinkedList* LinkedList;

LinkedList head;// global node


LinkedList Create_LinkedList( int sockfd){ // basic creation and default values of a node
  LinkedList temp = (struct LinkedList*) malloc(sizeof(struct LinkedList));
  //temp->thread = NULL; // we do this when we create the thread in connection_handler
  temp->newsockfd = sockfd;
  temp->next = NULL;
  return temp;
};

void LLremove(int sockfd){// this is where we remove the node and then make the thread be done since this is where it goes when you enter exit.
  LinkedList ptr = head;
  LinkedList prev = NULL;
  while(ptr != NULL){
    if(ptr->newsockfd == sockfd){
		if(prev == NULL){//first item
			head = ptr->next;
		}
		else{//middle or last item
		prev->next = ptr->next;
		}
		close(ptr -> newsockfd);//close the socket
		free(ptr);// free node
		pthread_exit(0);// end of thread
    }
    prev = ptr;
    ptr = ptr->next;
  }
}

LinkedList LLinsert( int sockfd){// very standard insert 
  LinkedList ptr = head;
  LinkedList prev = NULL;
  while(ptr != NULL){
    prev = ptr;
    ptr = ptr->next;
  }
  prev->next = Create_LinkedList(sockfd);
  return prev->next;
}

int sockfd;
bank banks[20];// we made a array of 20 structs that hold 20 accounts.
pthread_mutex_t lock;
pthread_mutex_t bank_Mutex = PTHREAD_MUTEX_INITIALIZER; // lock stuff


void sighandler() // this is a signal handler to free all the nodes and sockets in each node before it shuts down.
{
   printf("\nServer Shutting Down\n");
   
   // frees all the threads and sockets and nodes before shutdown
   LinkedList temp = head;		
   LinkedList temp2 = NULL;
   if(head != NULL)
   {
	   while(temp != NULL)
	   {
		   temp2 = temp->next;
		   close(temp->newsockfd);
		   free(temp);
		   temp = temp2;
	   }
   }
   close(sockfd);
	exit(0); 
}
void printAll(){ // this is where we print the stats of the server and all the accounts.
	if(pthread_mutex_trylock(&bank_Mutex) == 0){
		int counter = 0;
		printf("\nSystem Status:\n");
		while(counter < 20){// looks through all 20 indexs
		if(banks[counter] != NULL){
			printf("bank: %sbalance: %.2f", banks[counter]->name, banks[counter]->balance);
			if(banks[counter]->inFlag == 1){ // in 1 then it is in session so print IN SERVICE
				printf(", IN SERVICE");
			}
			printf("\n");
		}
		counter++;
		}
	printf("\n");
	pthread_mutex_unlock(&bank_Mutex);
	}
}

void *delayTimer(void *arg)// this is the 20 sec timer for the server stats of all the accounts
{
    while(1)
    {
        sleep(20);// the one thread that we sent to this function just sleeps for 20 secs before printing everything outp
        printAll();
    }
    return 0;
}

void error(const char *msg)// error msg for errno
{
    perror(msg);
    exit(1);
}

void * client_handler (void* input) // This is basically where every client interactes with. It reads and writes msg from and to every client
{ 									// depending on the read/ input it will make stage equal a certain value so that it can go to the right set of commands
	 int newsockfd = *( (int *)input ); 
	 int counter = 0;
	 int n;
  char buffer[256];
  char* temp2;
  char temp[107];
  char end[2] = "\n";
  char space[2] = " "; 
  temp[4] = '\0';
  int stage = 0;
  bank ptr = NULL;
  
     while(stage != -1){ //while connected to/reading from the client
		
       bzero(buffer,256);
		n = read(newsockfd,buffer,255);	       
			if (n < 0) 
			  error("ERROR reading from socket");
  
			temp2 = strtok(buffer, space);
		    if(temp2 == NULL)
			{
			}
			else
			if(stage == 0){ // this is stage 0 this is for the open, start and exit commands.
			  if(strcmp(temp2, "exit\n") == 0){ // everything for when a client types exit
				  
							printf("Client has been disconnected\n");
							n = write(newsockfd, "Disconnected\n", 13);
                            LLremove(newsockfd);
							//close(newsockfd);
							//pthread_exit(0);
                            break;
                          }
 			  else if(strcmp(temp2, "open") == 0){ // everything for when a client types open
				pthread_mutex_lock(&lock);
			    counter = 0;
			    temp2 = strtok(NULL, end);
				strcat(temp2, "\n");
				if(strlen(temp2) > 100)
				{
					counter = 35;
				}
                            while(counter < 20){
                              if(banks[counter] != NULL && strcmp(banks[counter]->name, temp2) == 0){
				counter = 22;
				break;
			      }
			      counter++;
			    }

			    if(counter != 22 && counter != 35)
			      counter = 0;
			    
			    while(counter < 20){
			      if(banks[counter] == NULL){
				banks[counter] = createBank(temp2);
				ptr = banks[counter];
				counter = 20;
				n = write(newsockfd, "Bank account has been made\n", 27);
			      }
			      counter++;
			    }
			    if(counter == 20){
			      n = write(newsockfd, "Bank accounts have reach max capacity\n", 38);
			    }
			    else if(counter == 22){
			      n = write(newsockfd, "Bank name already exists\n", 25);
			    }
				else if(counter == 35){
				  n = write(newsockfd, "Name needs to be under 100 characters\n", 38);
				}
			    else
			      stage = 1;
				pthread_mutex_unlock(&lock);
			  }
			  else if(strcmp(temp2, "start") == 0){ // everything for when a client types start
			    temp2 = strtok(NULL, end); 
				strcat(temp2, "\n");
			    counter = 0;
			    while(counter < 20){
			      if(banks[counter] != NULL && strcmp(banks[counter]->name, temp2) == 0){
				ptr = banks[counter];
				counter = 20;
			      }
			      counter++;
			    }
			    if(counter == 21){
					if(ptr->inFlag == 1){
						n = write(newsockfd, "Bank already in session\n", 24);
						
					}
					else{
			      n = write(newsockfd, "Bank successfully started\n", 26);
			      ptr->inFlag = 1;
			      stage = 1;
					}
			    }
			    else{
			      n = write(newsockfd, "No bank currently opened with specified name\n", 45);
			    }
			  }
			  else{
			    n = write(newsockfd, "Invalid input\n", 14);
			  }
			}
			else if(stage == 1 && ptr != NULL){ // this si stage 1 for credit, debit, balance, and finish commands.
			  if(strcmp(temp2, "credit") == 0){ // everything for when a client types credit we used strtok to get the number input
			    temp2 = strtok(NULL, space);
			    ptr->balance += atof(temp2);
			    n = write(newsockfd, "Credit added\n", 13);
			  }
			  else if(strcmp(temp2, "debit") == 0){// everything for when a client types debit  we used strtok to get the number input
			    temp2 = strtok(NULL, space);
			    if(ptr->balance < atof(temp2)){
			      n = write(newsockfd, "Not enough credit\n", 18);
			    }
			    else{
			      ptr->balance -= atof(temp2);
			      n = write(newsockfd, "Succesfully debited\n", 20);
			    }
			  }
			  else if(strcmp(temp2, "balance\n") == 0){ // everything for when a client types balance
			    char tempbalance[100];
			    sprintf(tempbalance, "Balance is currently: %.2f\n", ptr->balance);
			    n = write(newsockfd, tempbalance, strlen(tempbalance)+1); 
			  }
			  else if(strcmp(temp2, "finish\n") == 0){ // everything for when a client types finish
			    ptr->inFlag = 0;
			    ptr = NULL;
			    stage = 0;
			    n = write(newsockfd, "Customer session ended\n" , 23);
			  }
			  else{
			    n = write(newsockfd, "Invalid input\n", 14);
			  }
			}
			else{
			  n = write(newsockfd, "Invalid input\n", 14);
			}
     }
	close(newsockfd);
	pthread_exit(0);
}

// this function is where every connection happens, it is one thread that does on forever and when i finds a connection it creates a thread
//so the client will be sent to the client_handler for all the account stuff.
void * connection_handler(void* input)
{
	while(1)
	{/*
      time_t t;
      srand((unsigned) time(&t));
      int portno = (rand() % 12500) + 2500;
      printf("Server port is currently using :  %d\n", portno);
	  */
	  
	  int sockfdtemp = *( (int *)input );
      char buffer[256];
	  int newsockfd, n;
	  struct sockaddr_in cli_addr;
	  socklen_t clilen;
      clilen = sizeof(cli_addr);
	  newsockfd = accept(sockfdtemp, (struct sockaddr *) &cli_addr, &clilen);  // this accept is in a while(1) so the thread will always be looking for a connection
  if (newsockfd < 0) 
    error("ERROR on accept");
  else{
    write(newsockfd, "connection accepted",19 );
    bzero(buffer,256);
    n = read(newsockfd, buffer, 255);
    if (n < 0)
      error("ERROR reading from socket");
    printf("%s\n",buffer);
  }
  
      int * threadArgs0 = (int*)malloc(sizeof(int));          // so when a connection has been found it makes a thread and saves it in a struct along with the 
      *threadArgs0 = newsockfd; 							  // socket number for later useage
      //void* threadStatus0;
	  LinkedList temp;
      pthread_attr_t threadAttr0;
      pthread_attr_init(&threadAttr0);
      pthread_attr_setdetachstate(&threadAttr0, PTHREAD_CREATE_JOINABLE);
      pthread_attr_setscope(&threadAttr0, PTHREAD_SCOPE_SYSTEM);
	  
	  if(head == NULL)  // where we save the socket number
	  {
		temp = Create_LinkedList(newsockfd);
		head = temp;
	  }
	  else
	  {
		temp = LLinsert(newsockfd);
	  }
	  //where we save and create a thread to send the client to client_handler
      pthread_create(&temp->thread, &threadAttr0, client_handler , (void*) threadArgs0);
      pthread_attr_destroy(&threadAttr0);
      //pthread_join( thread0, &threadStatus0); 
	}
  pthread_exit(0);
}


// this is where we start up everything and where we create one thread for the connection_handler, this on thread will be in a infinite loop that will always
// look for a connection
int main(int argc, char *argv[])
{
  int counter = 0;
  while(counter < 20){
    banks[counter] = NULL;
    counter ++;
  }
	// for the signal handler
  signal(SIGINT, sighandler);
  
	//for 20 sec timer
	pthread_t timer;
	pthread_create(&timer, NULL, &delayTimer, NULL); 
  
  
   // this is in out main so that the connection_thread will not try to bind a socket to the server more then twice
	int portno = serverport; 
      struct sockaddr_in serv_addr;
      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0) error("ERROR opening socket");
      bzero((char *) &serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons(portno);
      if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) error("ERROR on binding");
      listen(sockfd,5);
	  
	  
	  
  
	int * threadArgs0 = (int*)malloc(sizeof(int));
    *threadArgs0 = sockfd;
	
	if (pthread_mutex_init(&lock, NULL) != 0)// where we init the mutex lock
    {
        printf("\n mutex init failed\n");
        return 1;
    }
  
  
  void* threadStatus0; // where we create the connection_handler thread
	pthread_t thread0;
	pthread_attr_t threadAttr0;
	pthread_attr_init(&threadAttr0);
	pthread_attr_setdetachstate(&threadAttr0, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&threadAttr0, PTHREAD_SCOPE_SYSTEM);
	pthread_create(&thread0, &threadAttr0, connection_handler ,(void*) threadArgs0);
	pthread_attr_destroy(&threadAttr0);
	pthread_join( thread0, &threadStatus0);
    pthread_mutex_destroy(&lock);
  
return 0;
}
// we finally DONEEEEEE!!!! last project whooooooooo!