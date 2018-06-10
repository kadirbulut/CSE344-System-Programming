#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <sys/time.h>
#include <time.h>


int main(int argc,char **argv){

    struct sockaddr_in serverAdress;
	struct timeval begin,end;
	int sock;
	int portNum;
	char localAdress[20];
	char message[250];
	char buffer[1024];
	char resultName[25];
	double resultCos;
	int resultPrice;
	double resultTime,totalTime;
	/*USAGE*/
	if(argc!=6){

		printf("USAGE: ./clientApp <clientName> <priorityParam> <degree> <serverAddress> <portNum>\n");
		exit(1);
	}

	srand(time(NULL));

	gettimeofday(&begin, NULL);


	/*create a socket*/
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("Client failed to create a socket\n");
		exit(1);
    }



	portNum=atoi(argv[5]);/*get number of port*/
	strcpy(localAdress,argv[4]); /*get ip of server*/


    serverAdress.sin_family = AF_INET;
    serverAdress.sin_port = htons(portNum);
	
	/*init the server adress*/
    if(inet_pton(AF_INET, localAdress, &serverAdress.sin_addr)<=0) 
    {
        printf("\nInvalid server address\n");
        return -1;
    }

	/*connect to server*/
    if (connect(sock, (struct sockaddr *)&serverAdress, sizeof(serverAdress)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
	memset(message,'\0',sizeof(message)/sizeof(int));
	strcpy(message,argv[1]); /*add client's name into message*/
	strcat(message," ");
	strcat(message,argv[2]);/*add client's priority paramater into message*/
	strcat(message," ");
	strcat(message,argv[3]);/*add clients's integer denoting degree into message*/

	printf("Client %s is requesting %s %s from server %s:%s\n",argv[1],argv[2],argv[3],argv[4],argv[5]);

	/*send message to server*/
    if(send(sock , message , strlen(message)+1,0) == -1){ 
		printf("Failed to sending message to server\n");
		if( close(sock) == -1){ /*close the socket*/
			printf("Failed to close the socket\n");
			exit(1);
		}
		exit(1);
	}

	read(sock,buffer,sizeof(buffer));

	/*in server is catched a signal and send an error result*/
	if(strcmp(buffer,"NO PROVIDER IS AVAILABLE")==0 || strcmp(buffer,"SERVER SHUTDOWN")==0){
		printf("%s's task can't be completed:%s\n",argv[1],buffer);
	}
	else{
		/*if server sends a suitable result*/
		sscanf(buffer,"%s %lf %d %lf",resultName,&resultCos,&resultPrice,&resultTime);
		printf("%s’s task completed by %s in %.3f seconds, cos(%s)=%.3f,cost is %dTL",argv[1],resultName,resultTime,argv[3],resultCos,resultPrice);
		gettimeofday(&end, NULL);
		totalTime= (double) (end.tv_usec - begin.tv_usec) / 1000000 +(double) (end.tv_sec - begin.tv_sec);
		printf(",total time spent %.3f seconds.\n",totalTime);
	}

	/*close the socket*/
	if( close(sock) == -1){
		printf("Failed to close the socket\n");
		exit(1);
	}

	return 0;

}


