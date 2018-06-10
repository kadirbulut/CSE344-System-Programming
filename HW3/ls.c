#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h> 


int main(int argc , char **argv){
	DIR *dirPtr; /* my directory pointer */
	struct dirent *dirEntryp; /* pointer to keep next entry*/
	FILE *f;
	char currentDir[255];

	int control ; 



	
	if(argv[1] == NULL)
		control=0;
	if(argv[1] != NULL && strcmp(argv[1],">") == 0)
		control=1;
	if(argv[1] != NULL && strcmp(argv[1],"|") == 0)
		control=1;
	
	if(control == 1)
		f = fopen(argv[2],"w");


	if(getcwd(currentDir,sizeof(currentDir)) == NULL ){
		perror("Couldn't get current directory!!!\n");
		if(control == 1)
			fclose(f);
		exit(EXIT_FAILURE);
	}

	if(	(dirPtr=opendir(currentDir))==NULL ){
		printf("Error while opening current directory!\n");		
		if(control == 1)
			fclose(f);
		exit(EXIT_FAILURE);
	}

	while((dirEntryp=readdir(dirPtr))!=NULL){

		if(strcmp(dirEntryp->d_name , ".") != 0 && strcmp(dirEntryp->d_name , "..") != 0 && strcmp(dirEntryp->d_name , "121044005fifo") != 0){
			if(control==0)
				printf("%s\n",dirEntryp->d_name);
			if(control==1)
				fprintf(f,"%s\n",dirEntryp->d_name);
		}

	}

	if(control == 1)
		fclose(f);


	return 0;
	
}





























