 #include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

const char *hostName [] = {"cs91515-1","cs91515-2","cs91515-3","cs91515-4","cs91515-5"};
const char portNo[] = "9000";

#define numberOfNodeControllers  4
#define numberOfProd  30
#define numberOfCons  50
#define numberOfDozens  50


pthread_t thread_id[(numberOfCons+numberOfProd)];
pthread_t threadContact[numberOfNodeControllers +1];
int acceptCount=1;
int ncID;// to be updated in the main function

int nodeControllers[numberOfNodeControllers - 1]; // number of node controllers -1
int ringBufferFD;
int timeStamp=0;
int clientIDCount=0;
int requestNumber = 0;
int ncConnectCount = 0;

pthread_mutex_t clientLock[8]; // for 4 donuts
pthread_mutex_t queueLock[8];
pthread_mutex_t prodLock;

pthread_cond_t clientID[numberOfProd + numberOfCons];

typedef struct  {
	int ts;
	int ncID;
	int rn;
	int msgType; // 0: request, 1:reply
	int replyCount; // shoulb be equal to numberOfNodeControllers-1 to proceed
	int client;
	int flavour;
	int clientType;
}msg;

msg pmsg;

struct queueNode {
	msg message;
	struct queueNode *next;
};

typedef struct {
	struct queueNode *first;
}linkedList;

linkedList list[8];



void addMessageToQueue(msg qmsg);
void server(int nodeNumber);
void client(int noOfConnect);
void *socketThread( void *arg);
void connectToBuffer();
void queueManager(msg qmsg);
void deleteMessageFromQueue(int flavour);


void *producer(void *arg){
	int flavour;
    int prodID=*(int *)arg;
	ushort  xsub1[3];
	struct timeval randtime;
	gettimeofday(&randtime, (struct timezone *)0);
	xsub1[0] = (ushort)randtime.tv_usec;
	xsub1[1] = (ushort)(randtime.tv_usec >> 16);
	xsub1[2] = (ushort)(getpid());


	while(1){
		flavour=nrand48(xsub1) & 3;

		pthread_mutex_lock(&clientLock[flavour]);
		pthread_mutex_lock(&queueLock[flavour]);
		// pmsg = malloc(sizeof(struct msg));
		pmsg.ts = timeStamp;
		timeStamp++;
		pmsg.ncID = ncID;
		pmsg.rn = requestNumber;
		requestNumber++;
		pmsg.msgType=0;
		pmsg.replyCount=0;
		pmsg.client= prodID;
		pmsg.flavour= flavour;
		pmsg.clientType=0;
		
		queueManager(pmsg);
		pthread_mutex_lock(&queueLock[flavour]);
		pthread_cond_wait(&clientID[pmsg.client],&clientLock[flavour]);
		write(ringBufferFD, pmsg, 8*sizeof(int));
		pthread_mutex_unlock(&clientLock[flavour]);
	}

}

void *consumer(void *arg){
	int flavour;
	int i,j; // loop counters 
	int consID= *(int *)arg;
	ushort  xsub1[3];
	struct timeval randtime;
	gettimeofday(&randtime, (struct timezone *)0);
	xsub1[0] = (ushort)randtime.tv_usec;
	xsub1[1] = (ushort)(randtime.tv_usec >> 16);
	xsub1[2] = (ushort)(getpid());


	while(1){

		for(i=0; i<numberOfDozens; i++){
			for(j=0; j<12; j++){

				flavour=nrand48(xsub1) & 3 + 4;
				pthread_mutex_lock(&clientLock[flavour]);
				pthread_mutex_lock(&queueLock[flavour]);
				// pmsg = malloc(sizeof(struct msg));
				pmsg.ts = timeStamp;
				timeStamp++;
				pmsg.ncID = ncID;
				pmsg.rn = requestNumber;
				requestNumber++;
				pmsg.msgType=0;
				pmsg.replyCount=0;
				pmsg.client= consID;
				pmsg.flavour= flavour;
				pmsg.clientType=1;
				
				queueManager(pmsg);
				pthread_mutex_unlock(&queueLock[flavour]);

				pthread_cond_wait(&clientID[pmsg.client],&clientLock[flavour]);
				//add it to ring buffer
				write(ringBufferFD, pmsg, 8*sizeof(int));

				pthread_mutex_unlock(&clientLock[flavour]);
			}
		}
	}
}

void addMessageToQueue(msg qmsg){
	// creating node to be inserted in the que
	struct queueNode newNode;
	newNode.message = qmsg;
	newNode.next= NULL;

	if(list[qmsg.flavour].first == NULL){
		list[qmsg.flavour].first= &newNode;
	}
	else{

		struct queueNode *current = list[qmsg.flavour].first;

		while(current->next != NULL && current->message.ts < newNode.message.ts ){
			current = current->next;
		}
		if(current->next !=NULL){
			while(current->message.ts == newNode.message.ts && current->message.ncID < newNode.message.ncID){
				current = current->next;
			}
			newNode.next = current->next;
			current->next = &newNode;
		}
		else{
			current->next = &newNode;
		}

	}

}

int main(int argc, char *argv[]){
	int i,threadCount;
	for (i = 0; i < 8; ++i)
	{
		list[i].first = NULL;
	}



	ncID = atoi(argv[1]);
	switch(ncID){

		case 1 :
			server(1);
			printf("In casee 1 server executed:\n");

			connectToBuffer();
			for(threadCount=0; threadCount< numberOfProd; threadCount++){
				pthread_create(&thread_id[threadCount],NULL,producer,(void *)&threadCount);
			}
			for(;threadCount< numberOfProd+numberOfCons; threadCount++){
				pthread_create(&thread_id[threadCount],NULL,consumer,(void *)&threadCount);
			}
	     	break ;

     	case 2 :
     		client(1);
     		server(2);
     		connectToBuffer();
			for(threadCount=0; threadCount< numberOfProd; threadCount++){
				pthread_create(&thread_id[threadCount],NULL,producer,(void *)&threadCount);
			}
			for(;threadCount< numberOfProd+numberOfCons; threadCount++){
				pthread_create(&thread_id[threadCount],NULL,consumer,(void *)&threadCount);
			}
     		break;
     	case 3 :
     		client(2);
     		server(3);
     		connectToBuffer();
			for(threadCount=0; threadCount< numberOfProd; threadCount++){
				pthread_create(&thread_id[threadCount],NULL,producer,(void *)&threadCount);
			}
			for(;threadCount< numberOfProd+numberOfCons; threadCount++){
				pthread_create(&thread_id[threadCount],NULL,consumer,(void *)&threadCount);
			}
     		break;
     	case 4 :
     		client(3);
     		connectToBuffer();
			for(threadCount=0; threadCount< numberOfProd; threadCount++){
				pthread_create(&thread_id[threadCount],NULL,producer,(void *)&threadCount);
			}
			for(;threadCount< numberOfProd+numberOfCons; threadCount++){
				pthread_create(&thread_id[threadCount],NULL,consumer,(void *)&threadCount);
			}
     		break;
	}
	
}

void server(int nodeNumber){
	int sockfd, newsockfd, portno, clilen;
	int ncPointer;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(portNo);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	    error("ERROR on binding");
	}
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	while(acceptCount < numberOfNodeControllers){
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
	    	error("ERROR on accept");
		}

	  	
	  	nodeControllers[ncConnectCount] = newsockfd;
	  	ncPointer = ncConnectCount;
	  	ncConnectCount++;
	  	

	  	pthread_create(&threadContact[acceptCount],NULL,socketThread,(void *)&ncPointer);
	  	acceptCount++;

	}
}

void client(int noOfConnect){
	int sockfd[noOfConnect];
	int ncPointer;
	int portno = atoi(portNo);
    struct sockaddr_in serv_addr[noOfConnect];
    struct hostent *server[noOfConnect];

	int i;
	for (i = 0; i<noOfConnect; i++){

		sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
	        error("ERROR opening socket");
		}

	    server[i] = gethostbyname(hostName[i]);
	    if (server[i] == NULL) {
	        fprintf(stderr,"ERROR, no such host\n");
	        exit(0);
    	}

    	bzero((char *) &serv_addr[i], sizeof(serv_addr[i]));
    	serv_addr[i].sin_family = AF_INET;
	    bcopy((char *)server[i]->h_addr,(char *)&serv_addr[i].sin_addr.s_addr,server[i]->h_length);
	    serv_addr[i].sin_port = htons(portno);
	    if (connect(sockfd[i],(struct sockaddr *)&serv_addr[i],sizeof(serv_addr[i])) < 0){error("ERROR connecting");}
        	
		printf("%d \n",sockfd[i]);
	  	nodeControllers[ncConnectCount] = sockfd[i];
	  	ncPointer = ncConnectCount;
		printf("%d \n",nodeControllers[ncPointer]);
	  	ncConnectCount++;
	  	pthread_create(&threadContact[i+1],NULL,socketThread,(void *)&sockfd[i]);
	  	acceptCount++;

	}
}


void *socketThread(void * arg ){
	while(1){
  		msg qmsg;
  		int ncPointer=*(int *)arg;

		int n = read(ncPointer,qmsg,(8*sizeof(int)));
		if (n < 0) error("ERROR reading from socket");
		queueManager(qmsg);
  	}
}

void connectToBuffer(){
	int ncPointer;
	int portno = atoi(portNo);
    struct sockaddr_in serv_addr;
    struct hostent *server;

	ringBufferFD = socket(AF_INET, SOCK_STREAM, 0);
	if (ringBufferFD < 0) {
        error("ERROR opening socket");
	}

    server = gethostbyname(hostName[4]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);
	
    if (connect(ringBufferFD,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
		{
				error("ERROR connecting");
		}
		pthread_create(&threadContact[4],NULL,socketThread,(void *)&ringBufferFD);
    	
}

void queueManager(msg qmsg){
	int i,n;

	if(qmsg.msgType==0){
		if(qmsg.ncID == ncID){
			addMessageToQueue(qmsg);
			for(i=0;i< numberOfNodeControllers-1; i++){
				n=write(nodeControllers[i],qmsg,(8*sizeof(int)));
			}
		}
		else{
			addMessageToQueue(qmsg);
		}
	}
	if((qmsg.msgType == 1)&&(qmsg.ncID == ncID)){
		struct queueNode *current = list[qmsg.flavour].first;
		while((current->message.ncID!=qmsg.ncID)||(current->message.ts!=qmsg.ts)){
			current=current->next;
		}
		current->message.replyCount=current->message.replyCount + 1;
		if (current->message.replyCount==numberOfNodeControllers-1){
			pthread_cond_signal(&clientID[current->message.client]);
		}
	}
	if(qmsg.msgType == 2){
		deleteMessageFromQueue(qmsg.flavour);
		struct queueNode *current = list[qmsg.flavour].first;
		while(current->message.ncID!=ncID){
			current->message.msgType=1;
			for(i=0; i< numberOfNodeControllers-1; i++){
				n=write(nodeControllers[i],current->message,(8*sizeof(int)));
			}
			deleteMessageFromQueue(qmsg.flavour);
			current = list[qmsg.flavour].first;
		}
	}
}

void deleteMessageFromQueue(int flavour){
	
	list[flavour].first = list[flavour].first->next;

}
