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

#define numberOfNodeControllers  4
#define numberOfFlavours  4
#define numberOfConsumers  50
#define numberOfProducers  30
#define noOfDonutSlots  500




pthread_mutex_t flavourLock[4];

pthread_cond_t flavourWait[4];
pthread_t thread_id[9];
pthread_t thread_id_Prod[4*numberOfProducers];
pthread_t thread_id_Cons[4*numberOfConsumers];
int ccount=0,pcount=0;


int ringBuffer[4][noOfDonutSlots];
int flavour_ptr[4];
int emptySlots[4];
int donutFlavourCount[4];
int donutAvailable[4];
int in_ptr[4];
int out_ptr[4];

int bigBasket[numberOfNodeControllers][numberOfConsumers][numberOfFlavours][12];
int count[numberOfNodeControllers*numberOfConsumers];

const char *hostName [] = {"cs91515-1","cs91515-2","cs91515-3","cs91515-4","cs91515-5"};
const char portNo[] = "9000";

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

typedef struct{
	int fd;
	msg qmsg;
}argInfo;

void server();
void *socketThread(void * arg );
void *producer(void *arg);
void *consumer(void *arg);

int nodeControllers[numberOfNodeControllers];

int main(int argc, char const *argv[]){
	int i;
	server();

	for(i=0; i<(numberOfConsumers*numberOfNodeControllers);i++){
		count[i]=0;
	}
	return 0;
}

void server(){
	int sockfd, newsockfd, portno, clilen, acceptCount=1;
	int ncPointer;
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
	while(acceptCount <= 4 ){
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
	    	error("ERROR on accept");
		}
		printf("%d \n",newsockfd);
	  	pthread_create(&thread_id[acceptCount],NULL,socketThread,(void *)&newsockfd);
		acceptCount++;
	}
}
void *socketThread(void * arg ){
	while(1){
  		msg qmsg;
  		argInfo data;
		

  		int ncPointer=*(int *)arg;
		int n = read(ncPointer,qmsg,(8*sizeof(int)));
		if (n < 0) error("ERROR reading from socket");
		data.fd=ncPointer;
		data.qmsg = qmsg;
		if(qmsg.clientType == 0){
			
			pthread_create(&thread_id_Prod[pcount],NULL,producer,(void *)&data);
			pcount++;
		}
		if(qmsg.clientType == 1){
			pthread_create(&thread_id_Cons[ccount],NULL,consumer,(void *)&data);
			ccount++;
		}
  	}
};
void *producer(void *arg){
	int fd; msg qmsg;
	argInfo data = *(argInfo*)arg;
	fd=data.fd;
	qmsg=data.qmsg;
	pthread_mutex_lock(&flavourLock[qmsg.flavour]);
	while(emptySlots[qmsg.flavour]==0){
		pthread_cond_wait(&flavourWait[qmsg.flavour],&flavourLock[qmsg.flavour]);
	}
	ringBuffer[qmsg.flavour][in_ptr[qmsg.flavour]]=donutFlavourCount[qmsg.flavour];
	donutFlavourCount[qmsg.flavour]++;
	emptySlots[qmsg.flavour]--;
	in_ptr[qmsg.flavour]=(in_ptr[qmsg.flavour]+1)%noOfDonutSlots;
	donutAvailable[qmsg.flavour]++;
	pthread_mutex_unlock(&flavourLock[qmsg.flavour]);
	pthread_cond_signal(&flavourWait[qmsg.flavour]);
	qmsg.msgType=2;
	write(fd,qmsg,(8*sizeof(int)));
}
void *consumer(void *arg){
	int fd; msg qmsg;
	argInfo data = *(argInfo*)arg;
	fd=data.fd;
	pthread_mutex_lock(&flavourLock[qmsg.flavour-4]);
	while(donutAvailable[qmsg.flavour-4]==0){
		pthread_cond_wait(&flavourWait[qmsg.flavour-4],&flavourLock[qmsg.flavour-4]);
	}
	bigBasket[qmsg.ncID-1][qmsg.client- numberOfProducers][qmsg.flavour-4][count[(qmsg.ncID)*numberOfConsumers]]=ringBuffer[qmsg.flavour-4][out_ptr[qmsg.flavour-4]];
	count[(qmsg.ncID)*numberOfConsumers]++;
	if(count[(qmsg.ncID)*numberOfConsumers]==12){
int i;
		for (i = 0; i < 12; ++i)
		{
			printf("bigBasket[qmsg.ncID-1][qmsg.client- numberOfProducers][qmsg.flavour-4][i]");
		}
		count[(qmsg.ncID)*numberOfConsumers]=0;
	}
	out_ptr[qmsg.flavour-4]= (out_ptr[qmsg.flavour-4]++)%noOfDonutSlots;
	emptySlots[qmsg.flavour-4]++;
	pthread_mutex_unlock(&flavourLock[qmsg.flavour-4]);
	pthread_cond_signal(&flavourWait[qmsg.flavour-4]);
	qmsg.msgType=2;
	write(fd,qmsg,(8*sizeof(int)));
}