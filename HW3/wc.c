#include <stdio.h>
#include <stdlib.h>
#include <string.h>




int main(int argc,char **argv){
	
	
	FILE *f,*fw;
	int count;
	char ch;
	int control;

	control=0;

	count=0;

	if(argv[2]!=NULL && strcmp(argv[2],">") == 0)
		control=1; /*to file*/

	if(control == 1)
		fw=fopen(argv[3],"w");

	f=fopen(argv[1],"r");
	
	if(f==NULL){
		printf("Invalid filename : %s\n",argv[1]);
		exit(-1);
	}
	
	if(feof(f)){
		printf("empty file!\n");
		fclose(f);
		if(control == 1){
			fprintf(fw,"0");
			fclose(fw);
		}
	}
	else{
		
		while(1){
			ch=fgetc(f);
			if(ch == '\n')
				count+=1;
			if(feof(f))
				break;
		}

	}
	


	if(control==0)
		printf("%d\n",count);
	
	if(control==1)
		fprintf(fw,"%d\n",count);

	fclose(f);
	if(control == 1)
		fclose(fw);




	return 0;
	
}





