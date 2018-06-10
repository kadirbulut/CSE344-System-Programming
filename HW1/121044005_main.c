#include "processor.h"

int main(int argc, char const *argv[]){
	
	FILE* fptr;
	unsigned char tempbuf[2];
	char type[5];
	/*check command line argumants,if there is a problem print an error message end exit*/
	if(argc!=2){
        printf("Usage: Command line error. Please enter 2 arguments!\n");
        printf("as -> ./tiffProcessor tiffFileName\n");
        return -1;
    }

	/*open the file in read mode*/			
	fptr=fopen(argv[1],"r");

	/*if filename paramater is invalid*/
	if(fptr == NULL ){
		printf("Error!File coudln't be found!!!\n");
		return -1;
	}
	
	/*check that input file's first bytes to know intel or motorola tif file*/
	if (fread(tempbuf,sizeof(unsigned char),2,fptr) == 0){
		printf("Error!Empty file!!!\n");
		return -1;	
	}
	/*get first bytes and print them into a string*/
	sprintf(type,"%02x%02x",tempbuf[0],tempbuf[1]);

	/*if file is intel */
	if(strcmp(type,"4949") == 0) 
		findIntel(fptr);

	/*if file is motorola*/
	if(strcmp(type,"4d4d") == 0) 
		findMotorola(fptr);	
	/*printf("%s\n",hexToBinary("f"));*/

	fclose(fptr); /*close input file*/
	return 0;
}	
