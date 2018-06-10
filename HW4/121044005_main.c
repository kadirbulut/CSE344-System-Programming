#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/shm.h>
#include <string.h>

/*flags and perms for named semaphore*/
#define PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)

/*sermaphore variables*/
const char* SEM_NAME = "/mysemaphore";
sem_t *mySem;


/*some control variables*/
int myIndex=0; /*0 for wholesaler and others for chefs*/
/*to keep ingredients*/
/*1 for eggs*/
/*2 for flour*/
/*3 for butter*/
/*4 for sugar*/
int ing1=0;
int ing2=0;


/*shared memory variables*/
key_t key;
int shmid;
char *sharedStr;



void createProcesses();
void printLacks(int index,int num1,int num2);
void ingredientsTrafic(int chef,int lack1,int lack2,int coming1,int coming2);
void street();
void signal_handler (int signum);
int getnamed(const char *name, sem_t **sem, int val);
int destroynamed(const char *name, sem_t *sem);


int main(int argc,char **argv){

	struct sigaction sa;


	if(argc!=1){
		printf("USAGE: ./exe\n");
		exit(EXIT_FAILURE);
	}

	srand(time(NULL)); /*for random num generator*/


	/*set CTRL^C signal*/

	sa.sa_handler = signal_handler;
  	sa.sa_flags = 0;

	if ((sigemptyset(&sa.sa_mask) == -1) ||(sigaction(SIGINT, &sa, NULL) == -1)){
		printf("Failed to install SIGTERM signal handler\n");
		exit(EXIT_FAILURE);
	}


	createProcesses();


	return 0;
}


void createProcesses(){

	int i;
	pid_t pid;
	FILE *fPtr;

	/*create file to keep child process' pids */
	fPtr=fopen("allPids","w");
	fclose(fPtr);

  
	
	for(i=0;i<6;i++){

		pid=fork();

		if(pid==0){

			myIndex = i+1; /*set index for chefs*/
	
			/*print pif into file*/
			fPtr=fopen("allPids","a");
			fprintf(fPtr,"%d\n",getpid());
			fclose(fPtr);

			switch(i+1){

				/*set 2 distinct ingredients for every chef according to combinations of 4 ingredients*/
				case 1:
					ing1=1; /*eggs*/
					ing2=2; /*flour*/
					break;

				case 2:
					ing1=1; /*eggs*/
					ing2=3;	/*butter*/
					break;

				case 3:
					ing1=1; /*eggs*/
					ing2=4;	/*sugar*/
					break;


				case 4:
					ing1=2; /*flour*/
					ing2=3;	/*butter*/ 
					break;


				case 5:
					ing1=2; /*flour*/
					ing2=4;	/*sugar*/
					break;


				case 6:
					ing1=3; /*butter*/
					ing2=4;	/*sugar*/
					break;

			}
				break;
		}

	}


	/*define the shared memory information for every process*/
	key = ftok("mySharedMemory",121044005);
	if( (shmid = shmget(key,3,0666|IPC_CREAT)) == -1){
		perror("creating shared memory error");
		exit(1);
	}
	

	/*if the process is main process (wholesaler) , initialize the shared memory to write with first character is 'w'*/
	if(myIndex == 0){
		sharedStr = (char*) shmat(shmid,(void*)0,0);
		sharedStr[0]='w';
	}

	/*define named semaphore for all processes*/
	if (getnamed(SEM_NAME, &mySem, 0) == -1) {
		perror("creating named semaphore	 error");
		exit(1);
	}

	/*if process is not wholesaler , go street function and wait there */
	if(myIndex > 0){
		street();
	}

	while(1){
			
		/*create 2 number between 1 - 4 and set wholesaler's ingredients according to them*/
		ing1 = ((rand() % 2) + 1);
		ing2= ((rand() % 2) + 3);
		
		/*for chefs (they are waiting in the street function) increase semaphore value 1 to run them*/
		if(myIndex==0){
			for(i=0;i<6;i++) /*6 is random here*/
				if (sem_post(mySem) == -1) {
					perror("Failed to unlock semlock");
					exit(1);
				}
		}

		for(i=0;i<99999;i++); /*wait a bit for chefs*/
		
		/*get shared memory*/
		if( (sharedStr = (char*) shmat(shmid,(void*)0,0)) == (void *)-1 ) { 
			perror("Failed to attach shared memory segment");
			exit(1);
		}

		/*if ingredients which were printed by wholesaler are taken by chefs ,write two random and distinct ingredients into shared memory again*/
		if(sharedStr[0] == 'w'){
			/*set first character to chefs (read )*/
			sharedStr[0] = 'r';
			/*set ingredients*/
			sharedStr[1] = ing1 + '0' ; 
			sharedStr[2] = ing2 + '0' ;

		}
	}	



	
}

void street(){
	
	int temp1,temp2;
	int control;

	while(1){
		
		/*lock the semaphore*/
		while (sem_wait(mySem) == -1){
			if (errno != EINTR) {
				perror("Failed to lock semlock");
				exit(1);
			}
		}

		printLacks(myIndex,ing1,ing2);/*print which ingredients a chef is waiting for*/

		sharedStr = (char*) shmat(shmid,(void*)0,0); /*get shared memory*/

		if(sharedStr[0] == 'r' ){ /*if wholesaler brings 2 new ingredients*/

			sem_wait(mySem);
			for(control=0;control<9999;control++);

			/*get ingredients*/
			temp1=sharedStr[1] - '0' ;
			temp2=sharedStr[2] - '0' ;
	

			/*check that ingredients are suitable to make şekerpare*/
			if(temp1!=ing1 && temp1!=ing2 && temp2!=ing1 && temp2!=ing2){

				ingredientsTrafic(myIndex,ing1,ing2,temp1,temp2); /*make şekerpare*/
				sharedStr[0]='w'; /*feedback for wholesaler*/

			}
		}
		
		/* sem post is not here , wholesaler is making post operation in other function */
	}

}	


void printLacks(int index,int num1,int num2){

	/*num1 : ingredient1*/
	/*num2 : ingredient2*/
	/* print which ingredients chefs are waiting for ?  if index > 0 */
	/* print which ingredients wholesaler has delivered ? if index=0 */ 
	if(num1==1 && num2 == 2){
		if(index!=0)
			printf("chef%d waiting for butter and sugar\n",index);
		else
			printf("wholesaler delivers eggs and flour\n");
	}
	
	if(num1==1 && num2 == 3){
		if(index!=0)		
			printf("chef%d waiting for flour and sugar\n",index);
		else
			printf("wholesaler delivers eggs and butter\n");
	}

	if(num1==1 && num2 == 4){
		if(index!=0)
			printf("chef%d waiting for flour and butter\n",index);
		else
			printf("wholesaler delivers eggs and sugar\n");
	}

	if(num1==2 && num2 == 3){
		if(index!=0)
			printf("chef%d waiting for eggs and sugar\n",index);
		else
			printf("wholesaler delivers flour and butter\n");
	}


	if(num1==2 && num2 == 4){
		if(index!=0)
			printf("chef%d waiting for eggs and butter\n",index);
		else
			printf("wholesaler delivers flour and sugar\n");
	}

	if(num1==3 && num2 == 4){
		if(index!=0)
			printf("chef%d waiting for eggs and flour\n",index);
		else
			printf("wholesaler delivers butter and sugar\n");

	}


}

void ingredientsTrafic(int chef,int lack1,int lack2,int coming1,int coming2){

	/*In this func wholesaler brings 2 ingredients */
	/*chef makes şekerpare*/
	/*wholesaler delivers it and goes back*/

	printLacks(0,coming1,coming2); /*wholesaler brought 2 ingredients with coming1 and coming2*/

	printf("chef%d has taken the ",chef);
	switch(coming1){
		case 1:
			printf("eggs\n");
			break;

		case 2:
			printf("flour\n");
			break;

		case 3:
			printf("butter\n");
			break;

		case 4:
			printf("sugar\n");
			break;
	}
	printf("wholesaler is waiting for the dessert\n");
	printf("chef%d has taken the ",chef);
	switch(coming2){
		case 1:
			printf("eggs\n");
			break;

		case 2:
			printf("flour\n");
			break;

		case 3:
			printf("butter\n");
			break;

		case 4:
			printf("sugar\n");
			break;
	}
	printf("chef%d is preparing the dessert\n",chef);
	printf("chef%d has delivered the dessert to the wholesaler\n",chef);
	printf("wholesaler has obtained the dessert and left to sell it\n");
	printLacks(chef,lack1,lack2);

}

void signal_handler (int signum)
{

	FILE *f;
	pid_t pid,c1,c2,c3,c4,c5,c6;
	int status;

	status=0;


	if(signum == SIGINT){

		if(myIndex == 0){

			printf("CTRL-C SIGNAL WAS RECEIVED.CHILD PROCESSES ARE KILLING AND PROGRAM IS TERMINATING...\n");

			/*initialize the child processes' pids*/
			c1=0;
			c2=0;
			c3=0;
			c4=0;
			c5=0;
			c6=0;

			/*read child processes' pids from file*/
			f=fopen("allPids","r");
			fscanf(f,"%d",&c1);
			fscanf(f,"%d",&c2);
			fscanf(f,"%d",&c3);
			fscanf(f,"%d",&c4);
			fscanf(f,"%d",&c5);
			fscanf(f,"%d",&c6);
			fclose(f);
			remove("allPids"); /*remove file*/

			/*send sigint signal to child processes and wait them one by one*/
			kill(c1,SIGINT);
			while ((pid = wait(&status)) > 0);

			kill(c2,SIGINT);
			while ((pid = wait(&status)) > 0);

			kill(c3,SIGINT);
			while ((pid = wait(&status)) > 0);

			kill(c4,SIGINT);
			while ((pid = wait(&status)) > 0);

			kill(c5,SIGINT);
			while ((pid = wait(&status)) > 0);

			kill(c6,SIGINT);
			while ((pid = wait(&status)) > 0);


			/*destroy named semaphore and shared memory objects*/

			destroynamed(SEM_NAME,mySem);

			if(shmdt(sharedStr) == -1){
				perror("shmdt shared memory error");
				exit(1);
			}
			if(shmctl(shmid,IPC_RMID,NULL) == -1){
				perror("shmctl shared memory error");
				exit(1);
			}


			exit(1); /*to terminate the parent process (wholesaler)*/

		}

	}
	
	/*this part for child processes*/
	destroynamed(SEM_NAME,mySem);
	if(shmdt(sharedStr) == -1){
		perror("shmdt shared memory error");
	}
	if(shmctl(shmid,IPC_RMID,NULL) == -1){
		perror("shmctl shared memory error");
		exit(1);
	}

	exit(1); /* to terminate the child processes*/
	
}

/*getnamed semaphore function from course book*/
/*reference : Unix System Programming page 590 , program 14.7*/
int getnamed(const char *name, sem_t **sem, int val) {
	while (((*sem = sem_open(name, FLAGS, PERMS, val)) == SEM_FAILED) && (errno == EINTR)) ;
	if (*sem != SEM_FAILED)
		return 0;
	if (errno != EEXIST)
		return -1;
	while (((*sem = sem_open(name, 0)) == SEM_FAILED) && (errno == EINTR)) ;
	if (*sem != SEM_FAILED)
		return 0;
	return -1;
}

/*destroynamed semaphore function from course book*/
/*reference : Unix System Programming page 594 , program 14.9*/
int destroynamed(const char *name, sem_t *sem) {
	int error = 0;
	if (sem_close(sem) == -1)
		error = errno;
	if ((sem_unlink(name) != -1) && !error)
		return 0;
	if (error)
	/* set errno to first error that occurred */
		errno = error;
	return -1;
}











