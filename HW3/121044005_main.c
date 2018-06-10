#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <signal.h>

/*terminal ayarlarını saklamak için*/
struct termios newTerm,term;

/*executable pathleri saklamak için*/
char catPath[100];
char lsPath[100];
char wcPath[100];

/*commandları historyde saklamak için*/
char history[200][50];
int historySize=0;
int historyIndex=0;

/*üst yön tuşuna basılan durumları kontrol etmek için*/
int upCommandControl=0;
int afterUpControl=0;
char afterUpInput[25];

/*histoyden command çalıştırma durumlarını kontrol etmek için*/
char afterHistoryInput[100];
int afterHistControl=0;

void signal_handler (int signum);
void myShell();
void saveExecutablePaths();
int callHistory(int index);
int parseInput(char tokensList[20][25],char input[255]);
void executeTokens(char tokensList[20][25],int tokensSize);
void upArrowKey();
void pwdCommand(int outType,char filename[25]);
void helpCommand();
void cdCommand(char newDir[25]);
void changeTerminalSettings();
void changeTerminalSettingsToOld();



int main(){

	struct sigaction sa;

	sa.sa_handler = signal_handler;
  	sa.sa_flags = 0;

	if ((sigemptyset(&sa.sa_mask) == -1) ||(sigaction(SIGTERM, &sa, NULL) == -1)){
		printf("Failed to install SIGTERM signal handler\n");
		exit(EXIT_FAILURE);
	}

	saveExecutablePaths(); /*mevcut executable pathleri kaydet*/
	

	myShell(); 

	return 0;
}


void myShell(){

	char first3Ch[5];
	char input[255];
	char tempInput[255];
	char tokensList[20][25];
	char ch[3];
	int control;
	int tokensSize;


	printf("********WELCOME TO MY SHELL*********\n");


	while(1){

		changeTerminalSettings();

/*bu etiket şunun için : historyde bir command seçildikten sonra execute etmek için ; gerekli kontrollerin yapılması ve
programın başa gelmesi gerekiyor*/
afterHistory:

		control=1;

		/*üst yön tuşuna basılıp basılmadığını kontrol et*/
		if((ch[0]=getchar()) == 27){
			if((ch[1]=getchar()) == 91){
				control+=1;
				if((ch[2]=getchar()) == 65){
					control+=1;
					upCommandControl+=1;
					upArrowKey();
					
				}
			}
		}
		/*üst yön tuşuyla yada history komutu ile bir command yeniden çalıştırılıyorsa*/
		else if(control==1 && ch[0]=='\n'){
			/*yön tuşuyla bir command seçilirse*/
			if(afterUpControl == 1){
				strcpy(input,afterUpInput);
				afterUpControl=0;
				upCommandControl=0;
				goto afterup;
			}
			/*historydeki bir command çalıştırılmak istenirse*/
			if(afterHistControl == 1){
				strcpy(input,afterHistoryInput);
				afterHistControl=0;
				goto afterup;
			}
			continue;

		}
		else{

			upCommandControl=0;
			changeTerminalSettingsToOld();
			/*kullanıcıdan command al*/
			fgets(tempInput, sizeof(tempInput), stdin);
			
			/*üst yön tuşunu kontrol ederken tuşa basılmadı ise kontrol edilen karakterleri de al stringin başına*/
			if(control==1)
				sprintf(first3Ch,"%c",ch[0]);
			if(control==2)
				sprintf(first3Ch,"%c%c",ch[0],ch[1]);
			if(control==3)
				sprintf(first3Ch,"%c%c%c",ch[0],ch[1],ch[2]);
			
			first3Ch[control+1] = '\0';
			
	

			strcpy(input,first3Ch);
			strcat(input,tempInput);

			/*input stringe null terminated ekle*/
			input[strlen(input)-1]='\0';

/*bu etiket üst yön tuşuyla yada historydeki bir command seçildiğinde*/
/*gerekli kontrollerden sonra kullanıcıdan yeni bir input alınmasını atlamak için*/
/*yeni bir command almak yerine var olan komut kullanılıyor*/
afterup:
			
			/*kullanıcıdan alınan inputu command listesine ekle ve histroyde sakla*/
			strcpy(history[historySize],input);
			historySize+=1;

			/*eğer kullanıcı exit yazarak shellden çıkmak isterse izin ver*/
			if(strcmp(input,"exit") == 0){
				break;
			}
			else{
				
		
				/*alınan input stringi parse ederek içerisindeki tokenları listeye at*/	
				tokensSize=parseInput(tokensList,input);

				/*kullanıcı historyden bir command mı çalıştırmak istiyor kontrol et*/
				if(strcmp(tokensList[0],"history") == 0 ){
					if(callHistory(atoi(tokensList[1])) == 1)
						goto afterHistory;
				}

				
				if(tokensSize > 0 ){
					/*geçerli bir command ise execute fonksiyonunu çağırarak execute et*/
					executeTokens(tokensList,tokensSize);
			
				}

			}
		}	

	}


}

void executeTokens(char tokensList[20][25],int tokensSize){

	int i;
	char newPath[255];
	int outType;
	pid_t pid;
	char myfifofile[25];

	for(i=0;i<tokensSize;i++){
		/******************************PWD COMMAND****************************/
		if(strcmp(tokensList[i],"pwd") == 0){
			if((i < tokensSize-1) && strcmp(tokensList[i+1],">") == 0){
				outType=1;
				pwdCommand(outType,tokensList[i+2]);
				i+=1;
			}
			else{
				outType=0;
				pwdCommand(outType,tokensList[i]);
			}
		}
		/******************************CD COMMAND****************************/
		else if(strcmp(tokensList[i],"cd") == 0){
			strcpy(newPath,tokensList[i+1]);
			cdCommand(newPath);
			i+=1;
		}
		/******************************HELP COMMAND****************************/
		else if(strcmp(tokensList[i],"help") == 0){
			helpCommand();
		}

		/******************************CAT COMMAND****************************/

		else if(strcmp(tokensList[i],"cat") == 0){
			
			pid=fork();
			
			if(pid < 0 ){
				printf("Fork failed\n");
				exit(1);
			}	
			
			if(pid == 0){
				/*redirecting > */
				if(i+3<= tokensSize && strcmp(tokensList[i+2],">")==0){
					execv(catPath,(char*[]){catPath,tokensList[i+1],tokensList[i+2],tokensList[i+3],NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/*redirecting < */
				if(i+1<= tokensSize && strcmp(tokensList[i+1],"<")==0){
					execv(catPath,(char*[]){catPath,tokensList[i+2],NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/*öncesinde pipe varsa*/
				else if(i!=0 && strcmp(tokensList[i-1],"|") == 0 ){
					strcpy(myfifofile,"121044005fifo");
					execv(catPath,(char*[]){catPath,myfifofile,NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/*sonrasında pipe varsa*/
				else if(i+2 <= tokensSize && strcmp(tokensList[i+2],"|") == 0 ){
					strcpy(myfifofile,"121044005fifo");
					execv(catPath,(char*[]){catPath,tokensList[i+1],">",myfifofile,NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/*normal command ise*/
				else{
					execv(catPath,(char*[]){catPath,tokensList[i+1],NULL});
					printf("Exec failed\n");
					exit(1);
				}
			}
			else
			{
				/*execute edilen tokenlara göre indexi ayarla*/
				if(i+3<= tokensSize && strcmp(tokensList[i+2],">")==0){
					i+=3;
				}
				else if(i+1<= tokensSize && strcmp(tokensList[i+1],"<")==0){
					i+=2;
				}
				else if(i!=0 && strcmp(tokensList[i-1],"|") == 0 ){
					i+=0;
				}
				else if(i+2 <= tokensSize && strcmp(tokensList[i+2],"|") == 0 ){
					i+=2;
				}
				else{
					i+=1;	
				}

				wait(NULL);
			
				/*öncesinde pipe var ise sonucu aldıktan sonra kaldır*/
				if(i!=0 && strcmp(tokensList[i-1],"|") == 0 )
					remove("121044005fifo");
			}


		}
		
		/******************************WC COMMAND****************************/

		else if(strcmp(tokensList[i],"wc") == 0){
			
			pid=fork();
			
			if(pid < 0 ){
				printf("Fork failed\n");
				exit(1);
			}	
			
			if(pid == 0){
				/*redirecting > varsa*/
				if(i+3<= tokensSize && strcmp(tokensList[i+2],">")==0){
					execv(wcPath,(char*[]){"wcc",tokensList[i+1],tokensList[i+2],tokensList[i+3],NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/*redirecting < varsa*/
				if(i+1<= tokensSize && strcmp(tokensList[i+1],"<")==0){
					execv(wcPath,(char*[]){wcPath,tokensList[i+2],NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/*öncesinde pipe varsa*/
				else if(i!=0 && strcmp(tokensList[i-1],"|")==0 ){
					strcpy(myfifofile,"121044005fifo");
					execv(wcPath,(char*[]){wcPath,myfifofile,NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/*sonrasında pipe varsa*/
				else if(i+2 <= tokensSize && strcmp(tokensList[i+2],"|")==0 ){
					strcpy(myfifofile,"121044005fifo");
					execv(wcPath,(char*[]){"wcc",tokensList[i+1],">",myfifofile,NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/*normal wc command ı ise*/
				else{
					execv(wcPath,(char*[]){wcPath,tokensList[i+1],NULL});
					printf("Exec failed\n");
					exit(1);
				}

			}
			else
			{
				/*execute edilen tokenlara göre indexi ayarla*/
				if(i+3<= tokensSize && strcmp(tokensList[i+2],">")==0)
					i+=3;
				else if(i+1<= tokensSize && strcmp(tokensList[i+1],"<")==0)
					i+=2;
				else if(i!=0 && strcmp(tokensList[i-1],"|")==0 )
					i+=0;
				else if(i+2 <= tokensSize && strcmp(tokensList[i+2],"|")==0 )
					i+=2;
				else
					i+=1;
				wait(NULL);

				/*öncesinde pipe varsa fifo dosyasıyla işi bittikten sonra dosyayı kaldır*/
				if(i!=0 && strcmp(tokensList[i-1],"|")==0 )
					remove("121044005fifo");
			}
	

		}


		/*******************************LS COMMAND****************************/
		
		else if(strcmp(tokensList[i],"ls") == 0){
						
			pid=fork();
			if(pid < 0 ){
				printf("Fork failed\n");
				exit(1);
			}	
			
			if(pid == 0){
				/*redirection >*/
				if(i+2<= tokensSize && strcmp(tokensList[i+1],">")==0){
					execv(lsPath,(char*[]){lsPath,tokensList[i+1],tokensList[i+2],NULL});
					printf("Exec failed\n");
					exit(1);
				}
				/* pipe | */
				if(i+2<= tokensSize && strcmp(tokensList[i+1],"|")==0){
					strcpy(myfifofile,"121044005fifo");
					execv(lsPath,(char*[]){lsPath,tokensList[i+1],myfifofile,NULL});
					printf("Exec failed\n");
					exit(1);
				}
				else{
					execv(lsPath,(char*[]){lsPath,NULL});
					printf("Exec failed\n");
					exit(1);
				}
			}
			else
			{
				/*execute edilen tokenlara göre indexi ayarla*/
				if(i+2<= tokensSize && strcmp(tokensList[i+1],">")==0)
					i+=2;			
				if(i+2<= tokensSize && strcmp(tokensList[i+1],"|")==0)
					i+=1;		

				wait(NULL);

			}

		}
		
	
		/**********************OTHER UNKNOWN COMMANDS*************************/
		else{
			printf("invalid command!!!\n");
		}
	}


}


int parseInput(char tokensList[20][25],char input[255]){
	
	int size,index;
	int i,j;


	/*kullanıcıdan gelen input stringi boşluk karakterine göre tokenlara ayır*/	
	for(i=0;i<20;i++)
		strcpy(tokensList[i],"");
	
	size=0;
	
	if(strcmp(input,"") == 0)
		return 0;		
	else{
		
		j=0;
		index=0;
		for(i=0;input[i]!='\0';i++){
			if(input[i] == ' '){	
				tokensList[index][j]='\0';
				j=0;
				index+=1;
				
				size+=1;
			}
			else{
				tokensList[index][j]=input[i];
				j+=1;
			}
		}
		tokensList[index][j]='\0';
		return size+1;
	}

}

void pwdCommand(int outType,char filename[25]){

	char currentDir[255];
	FILE *fptr;
	
	
	if(getcwd(currentDir,sizeof(currentDir)) == NULL ){
		perror("Couldn't found current directory!!!\n");
	}
	else{
		/*ekrana yaz*/
		if(outType == 0)
			printf("%s\n",currentDir);
		/*dosyaya aktar*/	
		if(outType == 1){
			fptr=fopen(filename,"w");
			fprintf(fptr,"%s",currentDir);
			fclose(fptr);
		}
	}
}

void cdCommand(char newDir[25]){


	char currentDir[255];	
	
	/*mevcut pathi al*/
	if(getcwd(currentDir,sizeof(currentDir)) == NULL ){
		printf("Couldn't found current directory!!!\n");
	}
	
	strcat(currentDir,"/");
	
	strcat(currentDir,newDir); /*yeni pathi ekle*/

	/*klasörü değiştir*/
	if (chdir(currentDir) == -1){
		printf("Failed to change current working directory");
	}
	


}

void helpCommand(){

	printf("**COMMANDS:******************************************************\n");
	printf("ls : list file type, access rights, file size and file name of all files\n");
	printf("pwd : prints the present working directory\n");
	printf("cd <location>: change the present working directory to the location\n");
	printf("help : list of supported commands\n");
	printf("cat <filename>: print on standard output the contents of the file\n");	
	printf("wc <filename>: print on standard output the number of lines in the file\n");
	printf("history n : prints the n-th previously command and waits 'ENTER' to execute it\n");
	printf("up arrow key: shows the previous executed command all time like a normal bash up arrow key\n");
	printf("<command1> | <command2>: pipe between command1 and command2.(command1's output is command2's input)\n");
	printf("> <param>: directs output to param\n");
	printf("<param> <: directs input to param\n");
	printf("exit : exit the shell\n");	
	printf("*****************************************************************\n");
}


int callHistory(int index){
	
	if(index > 0 && index <= historySize){
		afterHistControl=1;
		index=historySize-index;
		strcpy(afterHistoryInput,history[index]);
		printf("%s",history[index]);
		return 1;
	}
	else{
		printf("Index must be between %d and %d\n",1,historySize);
		return -1;
	}
}


void changeTerminalSettings(){

	/*terminali izlemek ve üst yön tuşuna basılıp basılmadığını kontrol etmek için*/
	tcgetattr(0, &term);          
	newTerm = term ;
	newTerm.c_lflag &= ~ICANON;      
	newTerm.c_cc[VMIN] = 1;   /*tek karakter için bekleme yap*/        
	newTerm.c_cc[VTIME] = 0;         
	tcsetattr(0, TCSANOW, &newTerm);

}

void changeTerminalSettingsToOld(){
 	tcsetattr(0, TCSANOW, &term); /*terminalin eski ayarlarını tekrar geri yükle*/
}

void upArrowKey(){

	char command[25];
	int index;

	if(historySize != 0){
		printf("\r                                ");

		if(upCommandControl <= historySize )
			index=historySize-upCommandControl;
		else if(upCommandControl%historySize == 0)
			index = 0 ;
		else	
			index=historySize-(upCommandControl%historySize);


		strcpy(command,history[index]);
	
		printf("\r%s",command);

		afterUpControl=1;
		strcpy(afterUpInput,command);
	}
}


void saveExecutablePaths(){
	char currentDir[100];
	
	if(getcwd(currentDir,sizeof(currentDir)) == NULL ){
		perror("Couldn't found current directory!!!\n");
		exit(1);
	}
	/*executable pathlerini sakla programın en başında*/
	strcpy(lsPath,currentDir);
	strcat(lsPath,"/lss");
	strcpy(wcPath,currentDir);
	strcat(wcPath,"/wcc");
	strcpy(catPath,currentDir);
	strcat(catPath,"/catt");
	
}

void signal_handler (int signum)
{
	pid_t pid;
	int status;
	status=0;
	if(signum == SIGTERM){
		
		/*childlar için işi bitene kadar bekle*/
		while ((pid = wait(&status)) > 0);

		fflush(stdout);
		printf("SIGTERM signal is catched . Program is terminating...\n");
		exit(1);
	}
}






