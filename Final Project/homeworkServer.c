#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>


#define PI 3.14159265359

/*to check program's execution time so all times are full or not*/
struct timeval startTime;
struct timeval tempTime;

FILE *fPtr;


int sock; /*socket descriptor to connect clients*/

int numOfDeadProviders=0; /*to keep number of dead providers*/

int signalFlag=0;

/*to keep a provider's datas*/
typedef struct{

	char name[25];
	int performance;
	int price;
	int duration;
	int isDead;/*provider is alive or not*/
	int exitSignal; /*ctrlc signal*/
	int queueIndex; /*it's own queue index*/
	int logoutFlag; /*to check duration time is full or not*/
}Provider;


Provider providers[100]; /*providers' struct array*/
int providersSize; 
pthread_t providersTids[100];
int cheapestList[100]; 
int bestPerformingList[100];
int leastBusyList[100];
int sizeOfQueues[100];
int providerQueues[100][2];
int totalStatistics[100];
int sleepNums[100];
double tempTimes[100];
char tempStr[100][15];
char taskCompletedStr[100][200];
/*to keep and check duration times from smaller to larger*/
int alarmList[100];
int alarmIndexes[100];


/*to keep a client's datas*/
typedef struct{

	char name[25];
	char priority;
	int degree;
	int sockFd;
	int resultIsReady;
	char result[500];
	struct timeval begin; /*to keep homework's start time*/
	struct timeval end; /*to keep homework's finish time*/
}Client;


Client clients[1000]; /*clients' struct array*/
int clientsSize;
pthread_t clientsTids[1000];



pthread_mutex_t myMutex; /*mutex for providers*/
pthread_mutex_t clientMutex; /*mutex for clients*/
pthread_mutex_t resultMutex; /*mutex for providers*/
pthread_cond_t providersCondValues[100]; /*condition variables for providers*/
pthread_cond_t clientsCondValues[1000]; /*condition variables for providers*/



/*the function reads all providers' datas from input file into a struct array*/
void getProvidersFromFile(char *filename); 
/*the function gets a provider's info from file*/
void getProviderInfo(char str[150]);
/*this function is a server's function , opens socket , get clients' requests,*/
/*cretaes a thread for each client request and checks duration times          */
void mainThreadServerFunction(int serverPort);
/*this function provider threads' function*/
void *providersPool(void *args); 
/*this function client threads' function*/
void *clientsFunc(void *args); 
/*this function creates cheapest , beset performing and least busy lists and  */
/*keeps these datas with provider indexes*/
void createPriorityLists();
/*This function creates a list with duration times from smaller to bigger and*/
/*sets an alarm with the biggest duration time*/
void setAlarmList();
/*this function checks duration times and if a provider's duration time is full*/
/*, deads it*/
void checkThatLogoutTimes();
/*this function checks that providers which are aliving and return a suitable*/
/*index for each request*/
int forwardSuitableProvider(char priority);
/*this function calcualates a cos value with given degree using Taylor series*/
/*and returns the degree's cos(degree) valeus*/
double findCosWithTaylor(int degree);
/*this function calculates given number's factorial value*/
double factorial(int n);
/*this function is a signal handle function*/
void signal_handler (int signum);





int main(int argc , char **argv){


	if(argc != 4){
		printf("USAGE: ./homeworkServer <connectionPort> <fileOfProviders> <logFile>\n");
		exit(1);

	}

	srand(time(NULL));
	getProvidersFromFile(argv[2]);

	if(providersSize == 0){
		printf("Empty file:%s\n",argv[2]);
		fprintf(fPtr,"Empty file:%s\n",argv[2]);
		fclose(fPtr);
		exit(1);
	}

	createPriorityLists();
	setAlarmList();

	fPtr=fopen(argv[3],"w"); /*open log file*/

	printf("Logs kept at %s\n",argv[3]);
	fprintf(fPtr,"Logs kept at %s\n",argv[3]);

	mainThreadServerFunction(atoi(argv[1]));

	return 0;
}



void mainThreadServerFunction(int serverPort){

	struct sigaction sa;
	struct sockaddr_in server;
	int error;
	int i;
	char buffer[1024];
	int n;
	int sockopt;
	int length;
	int newConnection;

	/*set signals*/
	sa.sa_handler = signal_handler;
  	sa.sa_flags = 0;

	if ((sigemptyset(&sa.sa_mask) == -1) || (sigaction(SIGINT, &sa, NULL) == -1) || (sigaction(SIGTERM, &sa, NULL) == -1) || (sigaction(SIGALRM, &sa, NULL) == -1)){
		printf("Failed to set signals\n");
		fprintf(fPtr,"Failed to set signals\n");
		exit(EXIT_FAILURE);
	}



	/*initialize mutex for providers*/
	if ((error = pthread_mutex_init(&myMutex, NULL))){
		fprintf(stderr, "Failed to initialize mutex for provider threads:%s\n", strerror(error));
		fprintf(fPtr,"Failed to initialize mutex for provider threads:%s\n", strerror(error));
		exit(1);
	}

	/*initialize mutex for clients*/
	if ((error = pthread_mutex_init(&clientMutex, NULL))){
		fprintf(stderr, "Failed to initialize mutex for client threads:%s\n", strerror(error));
		fprintf(fPtr,"Failed to initialize mutex for client threads:%s\n", strerror(error));
		exit(1);
	}

	/*initialize mutex for clients*/
	if ((error = pthread_mutex_init(&resultMutex, NULL))){
		fprintf(stderr, "Failed to initialize mutex for client threads:%s\n", strerror(error));
		fprintf(fPtr,"Failed to initialize mutex for client threads:%s\n", strerror(error));
		exit(1);
	}

	/*initialize control variables*/
	for(i=0;i<providersSize;i++){
		sizeOfQueues[i]=0;
		totalStatistics[i]=0;
		providerQueues[i][0]=-1;
		providerQueues[i][1]=-1;
	}

	/*initialize condition variables for providers*/
	for(i=0;i<providersSize;i++){
		if ((error = pthread_cond_init(&providersCondValues[i], NULL))){
			fprintf(stderr, "Failed to initialize condition variable for provider%d:%s\n",i+1,strerror(error));
			fprintf(fPtr, "Failed to initialize condition variable for provider%d:%s\n",i+1,strerror(error));
			exit(1);
		}
	}

	/*initialize condition variables for providers*/
	for(i=0;i<1000;i++){
		if ((error = pthread_cond_init(&clientsCondValues[i], NULL))){
			fprintf(stderr, "Failed to initialize condition variable for client%d:%s\n",i+1,strerror(error));
			fprintf(fPtr, "Failed to initialize condition variable for client%d:%s\n",i+1,strerror(error));
			exit(1);
		}
	}

	/*print providers' start datas*/
	printf("%d provider threads created\n",providersSize);
	fprintf(fPtr,"%d provider threads created\n",providersSize);
	printf("%-12s%-12s  %-12s%-12s\n","Name","Performance","Price","Duration");
	fprintf(fPtr,"%-12s%-12s  %-12s%-12s\n","Name","Performance","Price","Duration");
	for(i=0;i<providersSize;i++){
		printf("%-12s%-12d  %-12d%-12d\n",providers[i].name,providers[i].performance,providers[i].price,providers[i].duration);
		fprintf(fPtr,"%-12s%-12d  %-12d%-12d\n",providers[i].name,providers[i].performance,providers[i].price,providers[i].duration);
	}

	/*create threads for providers*/
	for(i=0;i<providersSize;i++){
		if ((error = pthread_create(&providersTids[i], NULL, providersPool, (void *)(long)i))){
			fprintf(stderr, "Failed to create thread: %s\n", strerror(error));
			fprintf(fPtr, "Failed to create thread: %s\n", strerror(error));
		}
		
	}


	clientsSize=0; /*set client size*/

	memset(&server,0,sizeof(struct sockaddr_in));

	/*create a socket*/
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("Failed to create socket\n");
		fprintf(fPtr,"Failed to create socket\n");
		exit(1);
	}

	/*attach the socket to the port*/
	sockopt=1;
	if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))) == -1) {
		printf("Failed to attaching socket to the port\n");
		fprintf(fPtr,"Failed to attaching socket to the port\n");
		if(close(sock) == -1){
			printf("Failed to closing the socket\n");
			fprintf(fPtr,"Failed to closing the socket\n");
		}
		exit(1);
	}

	/*set server options*/
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(serverPort);
	
	/*connect the given port to socket with bind*/
	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) == -1){
		printf("Failed to bind the socket to port\n");
		fprintf(fPtr,"Failed to bind the socket to port\n");
		exit(1);
	}

	/*listen the port which connected*/
	if(listen(sock, 10) == -1){
		printf("Failed to server to listen the port\n");
		fprintf(fPtr,"Failed to server to listen the port\n");
		exit(1);
	}

	length= sizeof(server);
	usleep(5000);
	printf("Server is waiting for client connections at port %d\n",serverPort);
	fprintf(fPtr,"Server is waiting for client connections at port %d\n",serverPort);

	while(1){

		if(numOfDeadProviders != providersSize){ /*if all providers are not dead*/
			n=0;
			/*accept a new client connection*/
			if((newConnection=accept(sock, (struct sockaddr*)&server, (socklen_t*)&length)) == -1){
				printf("Failed to accept connection\n");
				fprintf(fPtr,"Failed to accept connection\n");
			}
			memset(buffer,'\0',sizeof(buffer));
			/*read client's request*/
			if((n=read(newConnection, buffer, sizeof(buffer)-1)) == -1){
				printf("Failed to read client's request\n");
				fprintf(fPtr,"Failed to read client's request\n");
			}
			
			buffer[n] = 0;		
			checkThatLogoutTimes(); /*check that a provider's duration times is full*/

			/*get arguments from client's request*/
			sscanf(buffer,"%s %c %d",clients[clientsSize].name,&clients[clientsSize].priority,&clients[clientsSize].degree);
			clients[clientsSize].sockFd=newConnection; /*set socket descriptor for each client*/
			clients[clientsSize].resultIsReady=0;
			/*create a new thread for every new client connection*/
			if ((error = pthread_create(&clientsTids[clientsSize], NULL, clientsFunc, (void *)(long)clientsSize))){
				fprintf(stderr, "Failed to create thread: %s\n", strerror(error));
				fprintf(fPtr, "Failed to create thread: %s\n", strerror(error));
			}

			clientsSize+=1; /*increase client size's 1*/
			
		}

	}

}

void *providersPool(void *args){

	const int index = (long)(args); /*get index of provider*/
	int error;

	providers[index].queueIndex=0; /*set queue index to 0*/

	while(1){

		if(providers[index].exitSignal == 1){ /*if catching a signal break and exit*/
			break;
		}

		/*lock the mutex for a provider*/
		if ((error = pthread_mutex_lock(&myMutex))){
			fprintf(stderr, "Failed to lock the mutex:%s\n", strerror(error));
			fprintf(fPtr, "Failed to lock the mutex:%s\n", strerror(error));
			exit(1);
		}

		/*if a provider's queue is full so 2 task was completed by the provider , reset the it's queue*/
		if(providers[index].queueIndex == 2 && providers[index].queueIndex==2 ){
			providers[index].queueIndex=0;
			sizeOfQueues[index]=0;
		}


		/*wait for a condition variable with thread index*/
		if((sizeOfQueues[index] == 0 && providers[index].exitSignal==0 && providers[index].logoutFlag!=1) || (sizeOfQueues[index] == providers[index].queueIndex && providers[index].exitSignal==0 && providers[index].logoutFlag!=1)){
			if(sizeOfQueues[index] == 0){
				printf("Provider %s is waiting for a task\n",providers[index].name);
				fprintf(fPtr,"Provider %s is waiting for a task\n",providers[index].name);
			}
			/*wait with condition variable for a signal if there is not a task in the queue*/
			if((error = pthread_cond_wait (&providersCondValues[index], &myMutex))){
				fprintf(stderr, "Failed to wait to condition variable:%s\n", strerror(error));
				fprintf(fPtr, "Failed to wait to condition variable:%s\n", strerror(error));
				exit(1);
			}
		}

		/*check that a signal was received*/
		if(providers[index].exitSignal == 1){

			if ((error = pthread_mutex_unlock(&myMutex))){ /*unlock the mutex and go exit*/
				fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
				exit(1);
			}
			break;
		}
		/*check that all duration times are full*/
		if(providers[index].logoutFlag == 1 && (sizeOfQueues[index]==0 || (sizeOfQueues[index]==1 && providers[index].queueIndex==1))){

			if ((error = pthread_mutex_unlock(&myMutex))){ /*unlock the mutex and go exit*/
				fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
				exit(1);
			}
			break;
		}

		/*check that the provider's queue and index values*/
		if( sizeOfQueues[index] != 0 && sizeOfQueues[index]>providers[index].queueIndex && providerQueues[index][providers[index].queueIndex] != -1 
																									&& providers[index].exitSignal!=1 && providers[index].logoutFlag!=1){
			/*set homeworks start time for client*/
			gettimeofday(&clients[providerQueues[index][providers[index].queueIndex]].begin,NULL);
			printf("Provider %s is processing task number %d: %d\n",providers[index].name,providers[index].queueIndex+1,
																							clients[providerQueues[index][providers[index].queueIndex]].degree);
			fprintf(fPtr,"Provider %s is processing task number %d: %d\n",providers[index].name,providers[index].queueIndex+1,
																							clients[providerQueues[index][providers[index].queueIndex]].degree);
			/*calculate the cosinus value and print it client's result*/
			if(providers[index].exitSignal == 0){
				sprintf(clients[providerQueues[index][providers[index].queueIndex]].result,"%s %.3f %d",providers[index].name,
														findCosWithTaylor(clients[providerQueues[index][providers[index].queueIndex]].degree),providers[index].price);
				sprintf(taskCompletedStr[index],"Provider %s completed task number %d: cos(%d)=%.3f in ",providers[index].name,providers[index].queueIndex+1,
					clients[providerQueues[index][providers[index].queueIndex]].degree,findCosWithTaylor(clients[providerQueues[index][providers[index].queueIndex]].degree));
			}
		}

		/*unlock the mutex for an another thread*/
		if ((error = pthread_mutex_unlock(&myMutex))){
			fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
			fprintf(fPtr, "Failed to unlock the mutex:%s\n", strerror(error));
			exit(1);
		}

		sleepNums[index]=(rand()%11)+5;



		if(providers[index].exitSignal == 0){
			while(sleepNums[index] >0 && signalFlag != 1){
				sleep(1);
				sleepNums[index]-=1;
			}	
			if(signalFlag)
				return (void *)NULL;
			gettimeofday(&clients[providerQueues[index][providers[index].queueIndex]].end,NULL);
			if(providers[index].exitSignal == 0){
				
				/*get time of completed task*/
				tempTimes[index]= (double) (clients[providerQueues[index][providers[index].queueIndex]].end.tv_usec - 
					clients[providerQueues[index][providers[index].queueIndex]].begin.tv_usec) / 1000000 +(double) 
						(clients[providerQueues[index][providers[index].queueIndex]].end.tv_sec - clients[providerQueues[index][providers[index].queueIndex]].begin.tv_sec);

				clients[providerQueues[index][providers[index].queueIndex]].resultIsReady = 1; /*set result is ready value to 1*/

				/*lock the mutex for a provider to print task completed result*/
				if ((error = pthread_mutex_lock(&resultMutex))){
					fprintf(stderr, "Failed to lock the mutex:%s\n", strerror(error));
					fprintf(fPtr, "Failed to lock the mutex:%s\n", strerror(error));
					exit(1);
				}
			
					/*print task completed result to the screen and flog file*/
					printf("%s%.3f seconds.\n",taskCompletedStr[index],tempTimes[index]);
					fprintf(fPtr,"%s%.3f seconds.\n",taskCompletedStr[index],tempTimes[index]);


				/*unlock the mutex for an another thread after printting*/
				if ((error = pthread_mutex_unlock(&resultMutex))){
					fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
					fprintf(fPtr, "Failed to unlock the mutex:%s\n", strerror(error));
					exit(1);
				}

				/*send signal to client's cond values to give the result is ready's inform*/
				if((pthread_cond_signal(&clientsCondValues[providerQueues[index][providers[index].queueIndex]]))){
					fprintf(stderr, "Failed to send signal from provider to client:%s\n", strerror(error));
					fprintf(fPtr, "Failed to send signal from provider to client:%s\n", strerror(error));			
				} 
			}
			providers[index].queueIndex+=1;
			if(providers[index].exitSignal == 0)
				totalStatistics[index]+=1; /*increase own statistic value 1*/
		}
	}


	providers[index].isDead=1; /*set dead control value to 1*/
	return (void *)NULL; /*exit for a thread*/
}


void *clientsFunc(void *args){
	
	const int index = (long)(args); /*get index of client*/
	int error;
	int temp;
	int control;
	char str[15];
	double time;


	/*lock the mutex for a client*/
	if ((error = pthread_mutex_lock(&myMutex))){
		fprintf(stderr, "Failed to lock the mutex from client:%s\n", strerror(error));
		fprintf(fPtr, "Failed to lock the mutex from client:%s\n", strerror(error));
		exit(1);
	}	

	temp=forwardSuitableProvider(clients[index].priority);/*get suitable provider's index for client's request*/
	if(temp == -1){
		clients[index].resultIsReady=1;
		strcpy(clients[index].result,"NO PROVIDER IS AVAILABLE"); /*if there is no suitable provider*/
	}
	else{
		printf("Client %s (%c %d) connected, forwarded to provider %s\n",clients[index].name,clients[index].priority,clients[index].degree,providers[temp].name);
		fprintf(fPtr,"Client %s (%c %d) connected, forwarded to provider %s\n",clients[index].name,clients[index].priority,clients[index].degree,providers[temp].name);
		control=0;
		/*set request datas into struct array , print client index into suitable provider's queue*/
		if(sizeOfQueues[temp] == 0){
			providerQueues[temp][0] = index;
			sizeOfQueues[temp] +=1 ;
			control=1;
		}
		/*set request datas into struct array , print client index into suitable provider's queue*/
		if(sizeOfQueues[temp] == 1 && control==0){
			providerQueues[temp][1] = index;
			sizeOfQueues[temp] +=1 ;
		}
		if((error=pthread_cond_signal(&providersCondValues[temp]))){ /*send a signal to suitable provider's cond value*/
			fprintf(stderr, "Failed to send signal from client to provider:%s\n", strerror(error));
			fprintf(fPtr, "Failed to send signal from client to provider:%s\n", strerror(error));			
		}
	}

	/*unlock the mutex for an another thread*/
	if ((error = pthread_mutex_unlock(&myMutex))){
		fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
		fprintf(fPtr, "Failed to unlock the mutex:%s\n", strerror(error));
		exit(1);
	}

	
	/*lock the mutex for a client*/
	if ((error = pthread_mutex_lock(&clientMutex))){
		fprintf(stderr, "Failed to lock the mutex:%s\n", strerror(error));
		fprintf(fPtr, "Failed to lock the mutex:%s\n", strerror(error));
		exit(1);
	}


	/*wait until the result is prepared*/
	if(clients[index].resultIsReady == 0){
		if((error = pthread_cond_wait (&clientsCondValues[index], &clientMutex))){
			fprintf(stderr, "Failed to wait to condition variable:%s\n", strerror(error));
			exit(1);
		}
	}
	if(clients[index].resultIsReady == 2){
		strcpy(clients[index].result,"SERVER SHUTDOWN"); /*if a signal is received*/
	}
	else if(strcmp(clients[index].result,"NO PROVIDER IS AVAILABLE") != 0 && clients[index].resultIsReady == 1 ){
		time= (double) (clients[index].end.tv_usec - clients[index].begin.tv_usec) / 1000000 +(double) (clients[index].end.tv_sec - clients[index].begin.tv_sec);
		sprintf(str," %lf",time);
		strcat(clients[index].result,str);
	}

	if(signalFlag)
		strcpy(clients[index].result,"SERVER SHUTDOWN"); /*if a signal is received*/

	/*send the result on client's own socket connection descriptor*/
    send(clients[index].sockFd , clients[index].result , strlen(clients[index].result)+1,0); /*send message to server*/


	/*unlock the mutex for an another thread*/
	if ((error = pthread_mutex_unlock(&clientMutex))){
		fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
		fprintf(fPtr, "Failed to unlock the mutex:%s\n", strerror(error));
		exit(1);
	}
	
	return (void *)NULL;
}


void getProvidersFromFile(char *filename){

	FILE *fPtr;
	char line[150];

	fPtr = fopen(filename,"r"); /*open file to read*/

	if(fPtr == NULL){
		printf("Invalid file name to get providers!\n");
		fprintf(fPtr,"Invalid file name to get providers!\n");
		exit(1);
	}
	else{
		/*read providers until eof*/
		providersSize=0;
		fgets(line,sizeof(line),fPtr);
		while(fgets(line,sizeof(line),fPtr) && !feof(fPtr)){
			strcat(line,"\n");
			if(strcmp(line,"") != 0 && line[0]!='\n'){
				getProviderInfo(line);
				providersSize+=1;
			}
		}
		fclose(fPtr);
	}
	
}


void getProviderInfo(char str[150]){

	char name[25];
	char num[10];
	int i,j;

	/*get name from a line*/
	strcpy(name,"");
	for(i=0;str[i]!=' ';i++)
		name[i]=str[i];
	name[i] = '\0';

	strcpy(providers[providersSize].name , name );

	i++; /*jump space character*/


	/*get performance value*/
	j=0;
	strcpy(num,"");
	for(;str[i]!=' ';i++){
		num[j]=str[i];
		j+=1;
	}
	num[j]='\0';
	providers[providersSize].performance=atoi(num);

	i++; /*jump space character*/


	/*get price value*/
	j=0;
	strcpy(num,"");
	for(;str[i]!=' ';i++){
		num[j]=str[i];
		j+=1;
	}
	num[j]='\0';
	providers[providersSize].price=atoi(num);


	i++; /*jump space character*/

	/*get duration value*/
	j=0;
	strcpy(num,"");
	for(;str[i]!=' ';i++){
		num[j]=str[i];
		j+=1;
	}
	num[j]='\0';
	providers[providersSize].duration=atoi(num);


	providers[providersSize].isDead=0;
	providers[providersSize].exitSignal=0;
	providers[providersSize].logoutFlag=0;

}

void createPriorityLists(){

	int i,j;
	int temp[100],temp2[100];
	int size;
	int num;
	int controlFlag[100];

	size=providersSize;

	/*create an index array with prices from small to large for providers*/

	/*CHEAPEST LIST WITH PROVIDERS' INDEXES*/
	for(i=0;i<size;i++){
		temp[i] = providers[i].price;
		temp2[i] = providers[i].price;
	}
	
	for(i=0;i<size;i++){
		for(j=0;j<size;j++){
			if(temp[i] < temp[j]){
				num=temp[i];
				temp[i]=temp[j];
				temp[j]=num;

			}
		}
	}

	for(i=0;i<size;i++){
		controlFlag[i]=0;
	}

	for(i=0;i<size;i++){

		num=temp[i];
		
		for(j=0;j<size;j++)
			if(temp2[j] == num && controlFlag[j]!=1){
				controlFlag[j]=1;
				break;
			}

		cheapestList[i]=j;

	}

	/* BEST PERFORMING LIST WITH PROVIDERS' INDEXES */


	for(i=0;i<size;i++){
		temp[i] = providers[i].performance;
		temp2[i] = providers[i].performance;
	}

	for(i=0;i<size;i++){
		for(j=0;j<size;j++){
			if(temp[i] > temp[j]){
				num=temp[i];
				temp[i]=temp[j];
				temp[j]=num;

			}
		}
	}

	for(i=0;i<size;i++){
		controlFlag[i]=0;
	}


	for(i=0;i<size;i++){

		num=temp[i];
		
		for(j=0;j<size;j++)
			if(temp2[j] == num && controlFlag[j]!=1){
				controlFlag[j]=1;
				break;
			}

		bestPerformingList[i]=j;

	}

	/* LEAST DURATION LIST WITH PROVIDERS' INDEXES */


	for(i=0;i<size;i++){
		temp[i] = providers[i].duration;
		temp2[i] = providers[i].duration;
	}
	
	for(i=0;i<size;i++){
		for(j=0;j<size;j++){
			if(temp[i] < temp[j]){
				num=temp[i];
				temp[i]=temp[j];
				temp[j]=num;

			}
		}
	}

	for(i=0;i<size;i++){
		controlFlag[i]=0;
	}

	for(i=0;i<size;i++){

		num=temp[i];
		
		for(j=0;j<size;j++)
			if(temp2[j] == num && controlFlag[j]!=1){
				controlFlag[j]=1;
				break;
			}

		leastBusyList[i]=j;

	}

}

void setAlarmList(){
	
	int i,j;
	int temp,size;
	int controlFlag[100];

	size=providersSize;

	for(i=0;i<size;i++){
		alarmList[i]=providers[i].duration;
	}

	for(i=0;i<size;i++){
		for(j=0;j<size;j++){
			if(alarmList[j] > alarmList[i]){
				temp=alarmList[i];
				alarmList[i] = alarmList[j];
				alarmList[j]=temp;

			}

		}
	}
	for(i=0;i<providersSize;i++)
		controlFlag[i]=0;

	for(i=0;i<providersSize;i++){
		for(j=0;j<providersSize;j++){
			if(alarmList[i] == providers[j].duration && controlFlag[j]!=1){
				alarmIndexes[i] = j;
				controlFlag[j]=1;
				break;
			}
		}		


	}

	gettimeofday(&startTime,NULL);
	alarm(alarmList[size-1]);
}

void checkThatLogoutTimes(){

	int i;
	double currentTime;
	gettimeofday(&tempTime,NULL);

	currentTime= (double) (tempTime.tv_usec - startTime.tv_usec) / 1000000 +(double) (tempTime.tv_sec - startTime.tv_sec);

	for(i=0;i<providersSize;i++){

		if(currentTime >= (double)alarmList[i]){
			providers[alarmIndexes[i]].logoutFlag=1;
			numOfDeadProviders+=1;
			pthread_cond_signal(&providersCondValues[alarmIndexes[i]]);
		}

	}

}

int forwardSuitableProvider(char priority){

	int i;
	int size;
	
	size=providersSize;

	switch(priority){
		/*if client's priority is low cost*/
		case 'C':
			for(i=0;i<size;i++){
				
				if(sizeOfQueues[cheapestList[i]] != 2 && providers[cheapestList[i]].isDead != 1 && providers[cheapestList[i]].logoutFlag!=1){
					return cheapestList[i];
				}
			}			

			break;

		/*if client's priority is high performance*/
		case 'Q':
			for(i=0;i<size;i++){
				
				if(sizeOfQueues[bestPerformingList[i]] != 2 && providers[bestPerformingList[i]].isDead != 1 && providers[bestPerformingList[i]].logoutFlag != 1)
					return bestPerformingList[i];
			}			
			break;
		/*if client's priority is high speed*/
		case 'T':
			for(i=0;i<size;i++){ /*search providers which has queue size 0*/
				
				if(sizeOfQueues[leastBusyList[i]] != 2 && providers[leastBusyList[i]].isDead != 1 && providers[leastBusyList[i]].logoutFlag != 1)
					if(sizeOfQueues[leastBusyList[i]] ==0)
						return leastBusyList[i];
			}		
			for(i=0;i<size;i++){ /*search providers which has queue size 1*/
				
				if(sizeOfQueues[leastBusyList[i]] != 2 && providers[leastBusyList[i]].isDead != 1 && providers[leastBusyList[i]].logoutFlag != 1)
					if(sizeOfQueues[leastBusyList[i]] ==1)
						return leastBusyList[i];
			}			
			break;

	}


	return -1;
	
}

double findCosWithTaylor(int degree){

	double result;
	double x;
	int i;

	result=0;

	/*if degree bigger than 360*/
	if(degree > 360 )
		degree = degree % 360;

	/*if degree smaller than 0*/
	if (degree < 0 ){
		degree *= -1 ;
		degree = degree % 360 ;
		degree = 360-degree;

	}

	x= degree*(PI/180.0); /*convert degree to radian*/
	
	/*calculate cos(degree) with Taylor series formula*/
	for(i=0;i<=10;i++){
		result+=((pow(-1.0,i) / factorial (2*i)) * pow (x,2*i));
	}	

	return result;

}

double factorial(int n){

	int i;
	double result;

	result=1;

	for(i=n;i >=1 ; i--)
		result*=i ;


	return result;
	
}



void signal_handler (int signum){

	int i;
	int error;
	int *tempResult; 


	if(signum == SIGINT || signum == SIGTERM ){
		signalFlag=1;
		printf("Termination signal received\n");
		fprintf(fPtr,"Termination signal received\n");

		for(i=0;i<providersSize;i++){
			providers[i].exitSignal=1;
			providers[i].logoutFlag=1;
			sizeOfQueues[i]=0;
			pthread_cond_signal(&clientsCondValues[i]);
		}
		
		/*wait for clients*/
		for(i=0;i<clientsSize;i++){

			strcpy(clients[i].result,"SERVER SHUTDOWN");
			clients[i].resultIsReady=2;
			pthread_cond_signal(&clientsCondValues[i]);

		}

		printf("Terminating all clients\n");
		fprintf(fPtr,"Terminating all clients\n");

		/*wait for clients*/
		for(i=0;i<clientsSize;i++){
			if ((error = pthread_join(clientsTids[i], (void **)&tempResult))) {
				fprintf(stderr, "Failed to join clients' threads: %s\n", strerror(error));
			}
		}


		for(i=0;i<providersSize;i++){
			if(providers[i].isDead == 0){
				pthread_cond_signal(&providersCondValues[i]);
			}
		}

	
		printf("Terminating all providers\n");
		fprintf(fPtr,"Terminating all providers\n");
		/*wait for providers*/
		for(i=0;i<providersSize;i++){
			if ((error = pthread_join(providersTids[i],NULL))) {
				fprintf(stderr, "Failed to join providers' threads: %s\n", strerror(error));
			}
		}	


		/*destroy the mutex*/
		if ((error = pthread_mutex_destroy(&myMutex))){
			fprintf(stderr, "Failed to destroy mutex:%s\n", strerror(error));
		}

		/*destroy the mutex*/
		if ((error = pthread_mutex_destroy(&clientMutex))){
			fprintf(stderr, "Failed to destroy mutex:%s\n", strerror(error));
		}

		/*destroy the mutex*/
		if ((error = pthread_mutex_destroy(&resultMutex))){
			fprintf(stderr, "Failed to destroy mutex:%s\n", strerror(error));
		}

		/*destroy the condition variables for providers*/
		for(i=0;i<providersSize;i++){
			if ((error = pthread_cond_destroy(&providersCondValues[i]))){
				fprintf(stderr, "Failed to destroy condition variable for provider:%s\n",strerror(error));
				exit(1);
			}
		}

		/*destroy the condition variables for clients*/
		for(i=0;i<1000;i++){
			if ((error = pthread_cond_destroy(&clientsCondValues[i]))){
				fprintf(stderr, "Failed to destroy condition variable for provider:%s\n",strerror(error));
				exit(1);
			}
		}

		if(close(sock) == -1){
			printf("Failed to closing the socket\n");
			fprintf(fPtr,"Failed to closing the socket\n");
		}
	
		/*print statistics end of the program*/
		printf("Statistics\n");
		fprintf(fPtr,"Statistics\n");
		printf("%-12s          %s\n","Name","Number of clients served");
		fprintf(fPtr,"%-12s          %s\n","Name","Number of clients served");
		for(i=0;i<providersSize;i++){
			printf("%-12s          %d\n",providers[i].name,totalStatistics[i]);
			fprintf(fPtr,"%-12s          %d\n",providers[i].name,totalStatistics[i]);
		}
		printf("Goodbye.\n");
		fprintf(fPtr,"Goodbye.\n");
		fclose(fPtr);
		exit(1);

	}
	/*if signal is sig alarm so all providers' duration times are full*/
	if(signum == SIGALRM ){

		printf("All providers' duration times are full.\n");
		fprintf(fPtr,"All providers' duration times are full.\n");

		printf("Terminating all clients\n");
		fprintf(fPtr,"Terminating all clients\n");

		/*wait for all clients in the process*/
		for(i=0;i<clientsSize;i++){
			if ((error = pthread_join(clientsTids[i], (void **)&tempResult))) {
				fprintf(stderr, "Failed to join client's threads: %s\n", strerror(error));

			}
		}

		printf("Terminating all providers\n");
		fprintf(fPtr,"Terminating all providers\n");

		/*set all providers' exit variables*/
		for(i=0;i<100;i++){
			if(providers[i].isDead!=1){
				providers[i].exitSignal=0;
				providers[i].logoutFlag=1;
			}
		}	
	

		/*send signal to providers to logout them*/
		for(i=0;i<providersSize;i++){
			pthread_cond_signal(&providersCondValues[i]);
		}	

		/*wait for all providers*/
		for(i=0;i<providersSize;i++){
			if ((error = pthread_join(providersTids[i], (void **)&tempResult))) {
				fprintf(stderr, "Failed to join providers' threads %s: %s\n", providers[i].name,strerror(error));

			}
		}	

		/*destroy the providers' mutex*/
		if ((error = pthread_mutex_destroy(&myMutex))){
			fprintf(stderr, "Failed to destroy mutex for providers:%s\n", strerror(error));

		}

		/*destroy the clients' mutex*/
		if ((error = pthread_mutex_destroy(&clientMutex))){
			fprintf(stderr, "Failed to destroy mutex for clients:%s\n", strerror(error));

		}

		/*destroy the mutex*/
		if ((error = pthread_mutex_destroy(&resultMutex))){
			fprintf(stderr, "Failed to destroy mutex:%s\n", strerror(error));
		}


		/*destroy the providers' condition variables*/
		for(i=0;i<providersSize;i++){
			if ((error = pthread_cond_destroy(&providersCondValues[i]))){
				fprintf(stderr, "Failed to destroy condition variable for providers%d:%s\n", i+1,strerror(error));

			}
		}

		/*destroy the clients' condition variables*/
		for(i=0;i<1000;i++){
			if ((error = pthread_cond_destroy(&clientsCondValues[i]))){
				fprintf(stderr, "Failed to destroy condition variable for clients%d:%s\n", i+1,strerror(error));

			}
		}

		/*close the socket*/
		if(close(sock) == -1){
			printf("Failed to closing the socket\n");
			fprintf(fPtr,"Failed to closing the socket\n");
		}

		/*print statistics end of the program*/
		printf("Statistics\n");
		fprintf(fPtr,"Statistics\n");
		printf("%-12s          %s\n","Name","Number of clients served");
		fprintf(fPtr,"%-12s          %s\n","Name","Number of clients served");
		for(i=0;i<providersSize;i++){
			printf("%-12s          %d\n",providers[i].name,totalStatistics[i]);
			fprintf(fPtr,"%-12s          %d\n",providers[i].name,totalStatistics[i]);
		}
		printf("Goodbye.\n");
		fprintf(fPtr,"Goodbye.\n");
		fclose(fPtr);
		exit(1);

	}



}













