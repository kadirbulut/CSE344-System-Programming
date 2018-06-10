#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>


int floristsSize=0; /*number of florists in the input file*/
int clientsSize=0;  /*number of clients in the input file*/


/*struct to keep a florist's datas*/
typedef struct{

	char name[25];
	int coorX;
	int coorY;
	double clickTime;
	int flowersNum;
	char flowers[20][25];

}Florist;

/*struct to keep a client's datas*/
typedef struct{

	int coorX;
	int coorY;
	char request[25];

}Client;

/*struct to keep a result's datas which will be printed by each florist after delivery*/
typedef struct{
	char name[25];
	char flower[25];
	int client;
	int clientCoorX;
	int clientCoorY;
}Result;

Florist florists[10]; /*florists' struct array*/
Client clients[1000]; /*clients' struct array*/



pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER; /*mutex*/
pthread_cond_t florist1Cond = PTHREAD_COND_INITIALIZER; /*condition variable for florist 1*/
pthread_cond_t florist2Cond = PTHREAD_COND_INITIALIZER; /*condition variable for florist 2*/
pthread_cond_t florist3Cond = PTHREAD_COND_INITIALIZER; /*condition variable for florist 3*/




int flag=0; /*to keep an exit status between threads*/

int counter; /*to keep how many requests have been processed*/

/*to keep index of request array each of 3 florists*/
int index1;
int index2;
int index3;

/*to keep current size of requests array of each florists*/
int tempsize1;
int tempsize2;
int tempsize3;

/*to keep florist's total time statistics*/
int florist1TotalTime=0;
int florist2TotalTime=0;
int florist3TotalTime=0;


/*request arrays for all processes*/
int floristArray1[200];
int floristArray2[200];
int floristArray3[200];


/*main thread process requests in this function*/
void processRequests(); 
/*in this function threads waits requests , deliver flowers and exit when the requests are over*/
void *threadPool(void *args); 
/*this function reads a file which given as a command line argument*/
void readFile(const char *filename);
/*this function gets a florist's datas from file*/
void getFloristInfo(const char str[150]);
/*this function gets a client's datas from file*/
void getClientInfo(const char str[150]);
/*this function calculates a distance between two points with Euclidean distance formula*/
double findDistance(int x1,int y1,int x2,int y2);
/*this function finds the closest florist for given client*/
int findClosedFlorist(int x , int y ,char flower[25]);

int main(int argc,char **argv){

	/*USAGE*/
	if(argc != 2){
		printf("Usage: ./floristApp <filename>\n");
		exit(1);
	}

	readFile(argv[1]); /*read file*/
	processRequests(); /*process requests*/

	return 0;
}

void processRequests(){

	int i;
	int error;
	int temp;
	pthread_t floristTids[3]; /*thread tids for florists*/
	int *tempResult; 
	int results[3];  /*to keep return values of threads*/

	/*initialize mutex for florists*/
	if ((error = pthread_mutex_init(&myMutex, NULL))){
		fprintf(stderr, "Failed to initialize mutex for threads:%s\n", strerror(error));
		exit(1);
	}

	/*initialize condition variable for florist1*/
	if ((error = pthread_cond_init(&florist1Cond, NULL))){
		fprintf(stderr, "Failed to initialize condition variable for florist1:%s\n", strerror(error));
		exit(1);
	}

	/*initialize condition variable for florist2*/
	if ((error = pthread_cond_init(&florist2Cond, NULL))){
		fprintf(stderr, "Failed to initialize condition variable for florist1:%s\n", strerror(error));
		exit(1);
	}

	/*initialize condition variable for florist3*/
	if ((error = pthread_cond_init(&florist3Cond, NULL))){
		fprintf(stderr, "Failed to initialize condition variable for florist1:%s\n", strerror(error));
		exit(1);
	}


	counter=0; /*there aren't received any requests yet*/
	/*each of 3 threads isn't processed any request yet*/
	index1=0;
	index2=0;
	index3=0;
	/*florists' request arrays are empty */
	tempsize1=0;
	tempsize2=0;
	tempsize3=0;

	srand(time(NULL)); /*to generate different random numbers*/


	/*create threads for florists*/
	for(i=0;i<3;i++){
		if ((error = pthread_create(&floristTids[i], NULL, threadPool, (void *)(long)i))){
			fprintf(stderr, "Failed to create thread: %s\n", strerror(error));
		}
		
	}
	
	printf("%d florists have been created\n",floristsSize);
	printf("Processing requests\n");
	

	for(i=0;i<clientsSize;i++){ /*up clients size*/
		/*find the closest florist*/
		temp=findClosedFlorist(clients[i].coorX,clients[i].coorY,clients[i].request);
		
		/*if there is not a suitable florist for next request*/
		if(temp == -1){
			printf("Client%d's request is not available in the florists\n",i);

			if ((error = pthread_mutex_lock(&myMutex))){
				fprintf(stderr, "Failed to lock the mutex:%s\n", strerror(error));
				exit(1);
			}

			counter+=1;
			if(counter == clientsSize){ /*if there aren't any requests*/
				flag=1;
				if ((error = pthread_cond_signal(&florist1Cond))) {
					pthread_mutex_unlock(&myMutex);
					exit(1);
				}
				if ((error = pthread_cond_signal(&florist2Cond))) {
					pthread_mutex_unlock(&myMutex);
					exit(1);
				}
				if ((error = pthread_cond_signal(&florist3Cond))) {
					pthread_mutex_unlock(&myMutex);
					exit(1);
				}

			}

			if ((error = pthread_mutex_unlock(&myMutex))){
				fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
				exit(1);
			}

		}		
		else{
			/*always wait lock the mutex to to prevent conflict*/
			if ((error = pthread_mutex_lock(&myMutex))){
				fprintf(stderr, "Failed to lock the mutex:%s\n", strerror(error));
				exit(1);
			}
			if(temp==0){ /*if request is suitable for florist1*/
				floristArray1[tempsize1]=i; /*add clients's index into florist's request array*/
				tempsize1+=1;
				if ((error = pthread_cond_signal(&florist1Cond))) { /*send a signal to wake up florist 1*/
					pthread_mutex_unlock(&myMutex);
					exit(1);
				}
			}
			if(temp==1){/*if request is suitable for florist2*/
				floristArray2[tempsize2]=i;  /*add clients's index into florist's request array*/
				tempsize2+=1;
				if ((error = pthread_cond_signal(&florist2Cond))) { /*send a signal to wake up florist 2*/
					pthread_mutex_unlock(&myMutex);
					exit(1);
				}
			}
			if(temp==2){/*if request is suitable for florist3*/
				floristArray3[tempsize3]=i; /*add clients's index into florist's request array*/
				tempsize3+=1;
				if ((error = pthread_cond_signal(&florist3Cond))) { /*send a signal to wake up florist 3*/
					pthread_mutex_unlock(&myMutex);
					exit(1);
				}
			}

			if ((error = pthread_mutex_unlock(&myMutex))){ /*always unlock the mutex for threads after add a request*/
				fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
				exit(1);
			}
		}
		
	}


	/*wait for florists*/
	for(i=0;i<3;i++){
		if ((error = pthread_join(floristTids[i], (void **)&tempResult))) {
			fprintf(stderr, "Failed to join thread: %s\n", strerror(error));
			exit(1);
		}
		results[i]= *tempResult;
	}		


	printf("All requests processed.\n");
	for(i=0;i<3;i++){
		printf("%s closing shop.\n",florists[i].name);
	
	}


	/*destroy the mutex*/
	if ((error = pthread_mutex_destroy(&myMutex))){
		fprintf(stderr, "Failed to destroy mutex:%s\n", strerror(error));
	}

	/*destroy the condition variable for florist1*/
	if ((error = pthread_cond_destroy(&florist1Cond))){
		fprintf(stderr, "Failed to destroy condition variable for florist1:%s\n", strerror(error));
		exit(1);
	}

	/*destroy the condition variable for florist2*/
	if ((error = pthread_cond_destroy(&florist2Cond))){
		fprintf(stderr, "Failed to destroy condition variable for florist1:%s\n", strerror(error));
		exit(1);
	}

	/*destroy the condition variable for florist3*/
	if ((error = pthread_cond_destroy(&florist3Cond))){
		fprintf(stderr, "Failed to destroy condition variable for florist1:%s\n", strerror(error));
		exit(1);
	}

	/*print sale statistics end of the program*/
	printf("Sale statistics for today:\n");
	printf("-------------------------------------------------\n");
	printf("Florist         # of sales         Total time	 \n");
	printf("-------------------------------------------------\n");
	printf("%s              %d                 %dms          \n",florists[0].name,tempsize1,results[0]);
	printf("%s              %d                 %dms          \n",florists[1].name,tempsize2,results[1]);
	printf("%s              %d                 %dms          \n",florists[2].name,tempsize3,results[2]);
	printf("-------------------------------------------------\n");
}

void *threadPool(void *args){

	const int index = (long)(args);
	int tempTotalTime1,tempTotalTime2,tempTotalTime3;
	int error;
	Result result1,result2,result3;


	while(1){
		
		/*if all requests have been processed break the while loop */
		if(counter==clientsSize){
			flag=1;
			/*before break the while loop send a signal for other threads to give exit inform them*/
			if ((error = pthread_cond_signal(&florist1Cond))) {
				pthread_mutex_unlock(&myMutex);
				exit(1);
			}
			if ((error = pthread_cond_signal(&florist2Cond))) {
				pthread_mutex_unlock(&myMutex);
				exit(1);
			}
			if ((error = pthread_cond_signal(&florist3Cond))) {
				pthread_mutex_unlock(&myMutex);
				exit(1);
			}
			break;
		}

		/*lock the mutex for a thread*/
		if ((error = pthread_mutex_lock(&myMutex))){
			fprintf(stderr, "Failed to lock the mutex:%s\n", strerror(error));
			exit(1);
		}

		/*For florist1 */
		/*if there arent any request in florist1 array to process wait with florist1 condition variable*/
		if(((index==0 && index1 == tempsize1)  || (index==0 && tempsize1==0 && index1==0)) && counter!=clientsSize){
			if((error = pthread_cond_wait (&florist1Cond, &myMutex))){
				fprintf(stderr, "Failed to wait to condition variable:%s\n", strerror(error));
				exit(1);
			}
		}

		/*For florist2 */
		/*if there arent any request in florist2 array to process wait with florist2 condition variable*/
		if(((index==1 && index2 == tempsize2)  || (index==1 && tempsize2==0 && index2==0)) && counter!=clientsSize){
			if((error = pthread_cond_wait (&florist2Cond, &myMutex))){
				fprintf(stderr, "Failed to wait to condition variable:%s\n", strerror(error));
				exit(1);
			}
		}

		/*For florist3 */
		/*if there arent any request in florist2 array to process wait with florist2 condition variable*/
		if(((index==2 && index3 == tempsize3)  || (index==2 && tempsize3==0 && index3==0)) && counter!=clientsSize){
			if((error = pthread_cond_wait (&florist3Cond, &myMutex))){
				fprintf(stderr, "Failed to wait to condition variable:%s\n", strerror(error));
				exit(1);
			}
		}


		if(counter<clientsSize){
			/*********************CRITICAL SECTION STARTS********************/
			
			counter+=1;	/*increment counter 1*/

			/*FOR FLORIST 1*/
			if(index==0){
				/*get some client and florist datas from common arrays*/
				result1.client=floristArray1[index1]+1; /*get client's num*/
				strcpy(result1.name,florists[0].name); /*get florist's name*/
				strcpy(result1.flower,clients[floristArray1[index1]].request);
				result1.clientCoorX=clients[floristArray1[index1]].coorX; /*get client's coordinates*/
				result1.clientCoorY=clients[floristArray1[index1]].coorY;
				index1+=1; /*increment own index1 for own request array*/
			}
			/*FOR FLORIST 2*/
			if(index==1){
				/*get some client and florist datas from common arrays*/
				result2.client=floristArray2[index2]+1; /*get client's num*/
				strcpy(result2.name,florists[1].name); /*get florist's name*/
				strcpy(result2.flower,clients[floristArray2[index2]].request);
				result2.clientCoorX=clients[floristArray2[index2]].coorX; /*get client's coordinates*/
				result2.clientCoorY=clients[floristArray2[index2]].coorY;
				index2+=1; /*increment own index1 for own request array*/
			}

			/*FOR FLORIST 3*/
			if(index==2){
				/*get some client and florist datas from common arrays*/
				result3.client=floristArray3[index3]+1;  /*get client's num*/
				strcpy(result3.name,florists[2].name); /*get florist's name*/
				strcpy(result3.flower,clients[floristArray3[index3]].request);
				result3.clientCoorX=clients[floristArray3[index3]].coorX; /*get client's coordinates*/
				result3.clientCoorY=clients[floristArray3[index3]].coorY;
				index3+=1; /*increment own index1 for own request array*/
			}

			/*********************CRITICAL SECTION ENDS**********************/

		}
		
		/*unlock the mutex for an another thread*/
		if ((error = pthread_mutex_unlock(&myMutex))){
			fprintf(stderr, "Failed to unlock the mutex:%s\n", strerror(error));
			exit(1);
		}
		
		/*prepare and deliver a flower*/
		/*FLORIST 1*/
		if(index==0 && flag==0){
			tempTotalTime1= ((rand() % 41)+10); /*random preparation time between 10 and 50*/
			/*calculate time of delivery*/
			tempTotalTime1+= (int)(findDistance (florists[0].coorX,florists[0].coorY,result1.clientCoorX,result1.clientCoorY) * florists[0].clickTime);
			florist1TotalTime+=tempTotalTime1; /*add the time for own total time*/
			usleep(tempTotalTime1); /*sleep with time of preparation + time of delivery*/
			/*print the result*/
			printf("Florist %s has delivered a %s to client%d in %dms\n",result1.name,result1.flower,result1.client,(int)tempTotalTime1);
		}
		/*FLORIST 2*/
		if(index==1 && flag==0){
			tempTotalTime2= (rand() % 41)+10; /*random preparation time between 10 and 50*/
			/*calculate time of delivery*/
			tempTotalTime2+= (int)(findDistance (florists[1].coorX,florists[1].coorY,result2.clientCoorX,result2.clientCoorY) * florists[1].clickTime);
			florist2TotalTime+=tempTotalTime2; /*add the time for own total time*/
			usleep(tempTotalTime2); /*sleep with time of preparation + time of delivery*/
			/*print the result*/
			printf("Florist %s has delivered a %s to client%d in %dms\n",result2.name,result2.flower,result2.client,(int)tempTotalTime2);
		}
		/*FLORIST 3*/
		if(index==2 && flag==0){
			tempTotalTime3= (rand() % 41)+10; /*random preparation time between 10 and 50*/
			/*calculate time of delivery*/
			tempTotalTime3+= (int)(findDistance (florists[2].coorX,florists[2].coorY,result3.clientCoorX,result3.clientCoorY) * florists[2].clickTime);
			florist3TotalTime+=tempTotalTime3; /*add the time for own total time*/
			usleep(tempTotalTime3); /*sleep with time of preparation + time of delivery*/
			/*print the result*/
			printf("Florist %s has delivered a %s to client%d in %dms\n",result3.name,result3.flower,result3.client,(int)tempTotalTime3);
		}
		
	}

	if(index == 0)
		return (void *)&florist1TotalTime;
	else if(index == 1)
		return (void *)&florist2TotalTime;
	else
		return (void *)&florist3TotalTime;

}


void readFile(const char *filename){
	
	FILE *fPtr;
	char line[150];
	int flag;


	flag=0;

	fPtr=fopen(filename,"r");

	if(fPtr == NULL){
		printf("Opening file error!\n");
		exit(1);
	}
	else{


		while(fgets(line,sizeof(line),fPtr) && !feof(fPtr)){
				
			if(	line[strlen(line)-1] == '\n' )
				line[strlen(line)-1] = '\0' ;
			
			if(strlen(line) == 0)
				flag=1;
			if(flag == 0){
				getFloristInfo(line);
				floristsSize+=1;
			}
			if(flag == 1 && strlen(line) != 0){
				line[strlen(line)] = '\0';
				getClientInfo(line);
				clientsSize+=1;
				
				
			}
		}
	}

	fclose(fPtr);



	if(floristsSize==0 && clientsSize==0){
		printf("Couldn't find any florist or client\n");
		exit(1);
	}

	printf("Florist application initializing from file: %s\n",filename);

}



void getFloristInfo(const char str[150]){

	char name[25];
	char num[10];
	int i,j;
	int control;

	strcpy(name,"");
	for(i=0;str[i]!='(';i++)
		name[i]=str[i];
	name[i] = '\0';
	strcpy(florists[floristsSize].name , name );

	/* jump ( and find coordinat of X */
	j=0;
	strcpy(num,"");
	for(i=i+1;str[i]!=',';i++){
		num[j]=str[i];
		j+=1;
	}
	num[j]='\0';
	florists[floristsSize].coorX=atoi(num);

	/* jump , and find coordinat of Y */
	j=0;
	strcpy(num,"");
	for(i=i+1;str[i]!=';';i++){
		num[j]=str[i];
		j+=1;
	}
	num[j]='\0';
	florists[floristsSize].coorY=atoi(num);

	/* jump , and find coordinat of Y */
	j=0;
	strcpy(num,"");
	for(i=i+1;str[i]!=')';i++){
		num[j]=str[i];
		j+=1;
	}
	num[j]='\0';
	florists[floristsSize].clickTime=atof(num);

	/* jump to flower names*/
	while(str[i]==' ' || str[i]==':' || str[i]==')')
		i++;

	
	/*save flower names*/
	j=0;
	strcpy(name,"");
	control=0;

	florists[floristsSize].flowersNum=0;
	

	for(;str[i]!='\0';i++){

		if(str[i]!=',' && str[i]!=' '){
			name[j]=str[i];
			j+=1;


		}
		if(str[i] == ','){
label1:
			name[j]='\0';
			strcpy(florists[floristsSize].flowers[florists[floristsSize].flowersNum],name);
			florists[floristsSize].flowersNum+=1;
			j=0;
			strcpy(name,"");	
		}

	}

	if(control==0){
		control=1;
		goto label1;

	}


}


void getClientInfo(const char str[150]){
	
	int i,j;
	char temp[25];


	i=0;
	while(str[i] != '('){

		i+=1;

	}

	i+=1;
	
	j=0;
	strcpy(temp,"");
	for(;str[i]!=',';i++){
		temp[j]=str[i];
		j++;
	}
	temp[j]='\0';

	clients[clientsSize].coorX = atoi(temp);
	
	i+=1;

	j=0;
	strcpy(temp,"");
	for(;str[i]!=')';i++){
		temp[j]=str[i];
		j++;
	}
	temp[j]='\0';

	clients[clientsSize].coorY = atoi(temp);

	i+=1;

	while(str[i] == ' ' || str[i]==':')
		i+=1;

	j=0;
	strcpy(temp,"");
	for(;str[i]!='\0';i++){
		if(str[i]!=' '){
			temp[j]=str[i];
			j+=1;

		}

	}

	temp[j]='\0';

	strcpy(clients[clientsSize].request,temp);

	
}


double findDistance(int x1,int y1,int x2,int y2){

	double result;

	result=0;
	
	result = sqrt(pow(x2-x1,2)+pow(y2-y1,2));

	return result;

}


int findClosedFlorist(int x , int y , char flower[25]){
	
	double res[3];
	double temp;
	int control;
	int i;
	int j;
	int index;
	

	for(i=0;i<3;i++)
		res[i]=0;


	/*calculate distances between client and all florists*/
	for(i=0;i<3;i++){
		control=0;
		for(j=0;j<florists[i].flowersNum;j++){
			/*check that client's flower is exist between the florist's flowers*/
			if((strcmp(florists[i].flowers[j],flower) == 0) && control==0){
				control=1;
				if(control == 1){
					
					res[i] = findDistance(x,y,florists[i].coorX,florists[i].coorY);
					break;
				}
			}
		}	
		/*if there is not exist client's flower in the florist*/
		if(control == 0){

			res[i] = 9321.1239;/*I choosed this num random since this distance can be exist */
		
		}

	}

	index=-1;
	/*find first min distance*/
	for(i=0;i<3;i++){
		if(res[i] != 9321.1239){
			index=i;
			temp=res[i];
			break;
		}
	}	
	if(index == -1)
		return -1;
	/*find min distance between all distances*/
	for(i=0;i<3;i++){
		if(res[i] != 9321.1239 && res[i]<temp){
			index=i;
			temp=res[i];
		}
	}

	return index; /*return suitable florist's index in the florists array*/
}



























