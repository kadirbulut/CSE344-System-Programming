#include "processor.h"

void findIntel(FILE *fptr){
	unsigned char **arr; /*dynamic array to keep file's content (bytes)*/
    unsigned char buffer[16];
	unsigned char tempCh,tempCh1,tempCh2;
	char tempStr[15],tempStr2[200];
	char *ptr;
	char *pixels; /*pixel degerlerini tutmak için 0 lar ve 1 ler ile */ 
	long width,height; /*to keep image's width and height values*/
	int len;
	int i,j,k,m,n;
	size_t gotBytes;
	int counter,repeat,index,control;
	int bitControlType;

	/*go end of the file and get it's length and seek again start of the file*/
    fseek(fptr, 0 , SEEK_END); 
	len=ftell(fptr);
	rewind(fptr);

	/*allocate the memory with file's length*/
	arr = (unsigned char**)calloc(len , sizeof (unsigned char *));
	for(j=0;j<len;j++)
		arr[j]=(unsigned char*)calloc(2,sizeof(unsigned char));


	/*read file into 2D unsigned char array*/
	j=0;
	gotBytes=0;
	counter=0;
    while ((gotBytes = fread (buffer, 16, 1, fptr)) > 0 ) {
		counter++;
        for (i = 0; i < sizeof(buffer); i++){
			/*get bytes 16 by 16*/
			arr[j][0]=buffer[i];
			arr[j][1]=buffer[i+1];
			j+=1;
			i+=1;
		}
    }

	/*16sar 16sar tüm bytler okunduktan sonra arda kalan varmı kontrol et*/
	/*var ise onlarıda oku ve byte ları tuttugum dinamik arrayin sonuna ekle*/
	counter*=16;
    fseek( fptr,-(len-counter), SEEK_CUR );

    for (i = (counter/2); i < len/2; i++){	
		fread (&tempCh1,sizeof(unsigned char), 1, fptr);
		fread (&tempCh2,sizeof(unsigned char), 1, fptr);
		arr[i][0]=tempCh1;
		arr[i][1]=tempCh2;

	}

	/*tagleri okumak için byteları ters çevir çünkü intel ters dizilişini saklıyor*/
	for(i=4 ; i<(len/2) ; i++){	
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		tempCh=arr[i][0];
		arr[i][0]=arr[i][1];
		arr[i][1]=tempCh;
	}


	/*width height bulma*/
	width=-1;
	height=-1;
	for(i=0 ; i<(len/2) ; i++){
				
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		/*0100 tagiyle width yi bul*/
		if( strcmp(tempStr,"0100") == 0){
			sprintf(tempStr,"%02x%02x%02x%02x%02x%02x",arr[i+1][0],arr[i+1][1],arr[i+2][0],arr[i+2][1],arr[i+3][0],arr[i+3][1]);
			if(strcmp(tempStr,"000300010000") == 0){
				sprintf(tempStr,"%02x%02x",arr[i+4][0],arr[i+4][1]);
				width=strtol(tempStr,&ptr,16); /*stringden 16 lık tabandan widthyi elde et*/
			}

		}
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		/*0101 tagiyle height bul*/
		if( strcmp(tempStr,"0101") == 0){
			sprintf(tempStr,"%02x%02x%02x%02x%02x%02x",arr[i+1][0],arr[i+1][1],arr[i+2][0],arr[i+2][1],arr[i+3][0],arr[i+3][1]);
			if(strcmp(tempStr,"000300010000") == 0){
				sprintf(tempStr,"%02x%02x",arr[i+4][0],arr[i+4][1]);
				height=strtol(tempStr,&ptr,16); /*stringden 16 lık tabandan hegiht i  elde et*/
			}
		}

		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		if(width!=-1 && height!=-1)
			break;
	}
	/*width height ve byte orderı yazdır*/
	printf("Width : %ld pixels\nHeigt : %ld pixels\n",width,height);
	printf("Byte order : Intel\n");

	/*pixel değerlerinin arrayde byte olarak mı bit olarak mı saklandığını bul*/
	/*bunun için 0102 tagini kullan*/
	for(i=0 ; i<(len/2) ; i++){
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		if( strcmp(tempStr,"0102") == 0){
			sprintf(tempStr,"%02x%02x",arr[i+4][0],arr[i+4][1]);
			bitControlType=strtol(tempStr,&ptr,16);/*kaç bit oalrak tutulduğu*/
		}
	}


	/*ters çevrilen byte değerlerini tekrar ters çevir pixelleri okumak için*/
	for(i=4 ; i<(len/2) ; i++){	
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		tempCh=arr[i][0];
		arr[i][0]=arr[i][1];
		arr[i][1]=tempCh;
	}

	/*pixel degerlerini 0 1 olarak tutmak için array allocate et width*height */
	pixels = (char*)calloc((width*height)+1,sizeof(char));
	for(i=0;i<width*height;i++)
		pixels[i]='2';
	pixels[width*height]='\0';



	/*eger pixeller 1er 1 olarak saklanıyorsa */
	if(bitControlType == 1 ){

		i=4;/*pixel degerlerinin baslangıc indexi*/
		repeat=(width/16)+1; /*kac tane 16lık grup kullanılacak*/
		index=0;
		strcpy(tempStr2,"");
		for(j=0;j<height;j++){
			strcpy(tempStr2,"");
			for(k=0;k<repeat;k++){
				sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
				/*sırası ile her 4 karakterin binary acılımını bul ve*/
				/*bir stringde sakla*/
				for(m=0;m<4;m++){
					strcat(tempStr2,hexToBinary(tempStr[m]));
				}
				i+=1;
			}
			/*her 16 lık grupta bir pixel değerlerini al saklanan stringden*/
			for(n=0;n<width;n++){
				pixels[index]=tempStr2[n];
				index+=1;
			}

		}
		
		/*print pixel results with 0's and 1's*/
		for(i=0;i<width*height;i++){
			if(i!=0 && i%(width) == 0)
				printf("\n");
			printf("%c",pixels[i]);
		}
		printf("\n");

	}


	/*eğer her pixel için 8 bit ayrılmış ve o sekilde saklanıyorsa*/
	if(bitControlType == 8 || bitControlType == 0 ||bitControlType!=1){
		j=0;
		control=0;
		for(i=4 ; i<(width*height/2)+4;i++){

			if(j!=0 && j%(width)==0){
				j=0;
				printf("\n");		
			}
			/*her ff değeri yani beyaz için 1 yazdır*/
			sprintf(tempStr,"%02x",arr[i][0]);	
			sprintf(tempStr2,"%02x",arr[i][1]);
			if(strcmp(tempStr,"ff") == 0){
				printf("1");
				j+=1;
			}
			/*her 00 değeri yani siyah için 0 yazdır*/
			if(strcmp(tempStr,"00") == 0){
				printf("0");
				j+=1;
			}					
			if(strcmp(tempStr2,"ff") == 0){
				if(j==width && width%2==1){
					printf("\n1");
					j=1;
				}
				else{
					printf("1");
					j+=1;
				}
			}
			if(strcmp(tempStr2,"00") == 0){
				if(j==width && width%2==1){
					printf("\n0");
					j=1;
				}
				else{
					printf("0");
					j+=1;
				}
			}	
			control+=1;
		}
		if(width%2 == 1){
			sprintf(tempStr,"%02x",arr[i][0]);	
			if(strcmp(tempStr,"ff") == 0)
				printf("1");
			if(strcmp(tempStr,"00") == 0)
				printf("0");
		}
		printf("\n");
	}












	/*deallocate the allocated memories*/
	for ( j = 0; j < len; j++ )
		free(arr[j]);
	free(arr);
	free(pixels);

}


void findMotorola(FILE *fptr){

	unsigned char **arr; /*dynamic array to keep file's content (bytes)*/
    unsigned char buffer[16];
	unsigned char tempCh1,tempCh2;
	char tempStr[15],tempStr2[200];
	char *ptr;
	char *pixels; /*pixel degerlerini tutmak için 0 lar ve 1 ler ile */ 
	int len; /*to keep input file's length*/
	int i,j,k,m,n;
	int counter;	
	size_t gotBytes;
	int pixelStartIndex; /*pixels start index in the byte array*/
	int bitControlType; /*pixellerin arrayde kac bit olarak tutuldugunu tumak ıcın*/
	long width,height; /*to keep image's width and height values*/
	int index,repeat,control;

	/*go end of the file and get it's length and seek again start of the file*/
    fseek(fptr, 0 , SEEK_END); 
	len=ftell(fptr);
	rewind(fptr);

	/*allocate the memory with file's length*/
	arr = (unsigned char**)calloc(len , sizeof (unsigned char *));
	for(j=0;j<len;j++)
		arr[j]=(unsigned char*)calloc(2,sizeof(unsigned char));


	/*read file into 2D unsigned char array*/
	j=0;
	gotBytes=0;
	counter=0;
    while ((gotBytes = fread (buffer, 16, 1, fptr)) > 0 ) {
		counter++;
        for (i = 0; i < sizeof(buffer); i++){
			/*get bytes 16 by 16*/
			arr[j][0]=buffer[i];
			arr[j][1]=buffer[i+1];
			j+=1;
			i+=1;
		}
    }


	/*16sar 16sar tüm bytler okunduktan sonra arda kalan varmı kontrol et*/
	/*var ise onlarıda oku ve byte ları tuttugum dinamik arrayin sonuna ekle*/
	counter*=16;
    fseek( fptr,-(len-counter), SEEK_CUR );

    for (i = (counter/2); i < len/2; i++){	
		fread (&tempCh1,sizeof(unsigned char), 1, fptr);
		fread (&tempCh2,sizeof(unsigned char), 1, fptr);
		arr[i][0]=tempCh1;
		arr[i][1]=tempCh2;

	}

	/*pixel değerlerinin hangi indexten basladigini bul*/
	/*bunun için 0111 tagini kullan*/		
	for(i=0 ; i<(len/2) ; i++){
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		if( strcmp(tempStr,"0111") == 0){
			sprintf(tempStr,"%02x%02x",arr[i+5][0],arr[i+5][1]);
			pixelStartIndex=strtol(tempStr,&ptr,16);
			/*bulunan değeri 2 ye böl.(arrayde değerleri 16 sar tuttuğumdan dolayı)*/
			pixelStartIndex/=2; /*pixellerin başladığı değer */
		}
	}
	
	/*pixel değerlerinin arrayde byte olarak mı bit olarak mı saklandığını bul*/
	/*bunun için 0102 tagini kullan*/
	for(i=0 ; i<(len/2) ; i++){
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		if( strcmp(tempStr,"0102") == 0){
			sprintf(tempStr,"%02x%02x",arr[i+4][0],arr[i+4][1]);
			bitControlType=strtol(tempStr,&ptr,16);/*kaç bit oalrak tutulduğu*/
		}
	}


	/*width height bulma*/
	width=-1;
	height=-1;
	for(i=0 ; i<(len/2) ; i++){
				
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		/*to find width*/
		/*width bulmak için 0100 tagini kullan*/
		if( strcmp(tempStr,"0100") == 0){
			sprintf(tempStr,"%02x%02x%02x%02x%02x%02x",arr[i+1][0],arr[i+1][1],arr[i+2][0],arr[i+2][1],arr[i+3][0],arr[i+3][1]);
			if(strcmp(tempStr,"000300000001") == 0){
				sprintf(tempStr,"%02x%02x",arr[i+4][0],arr[i+4][1]);
				width=strtol(tempStr,&ptr,16);

			}
			if(strcmp(tempStr,"000400000001") == 0){
				sprintf(tempStr,"%02x%02x",arr[i+5][0],arr[i+5][1]);
				width=strtol(tempStr,&ptr,16);

			}


		}
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		/*to find height*/
		/*height bulmak için 0101 tagini kullan*/
		if( strcmp(tempStr,"0101") == 0){
			sprintf(tempStr,"%02x%02x%02x%02x%02x%02x",arr[i+1][0],arr[i+1][1],arr[i+2][0],arr[i+2][1],arr[i+3][0],arr[i+3][1]);
			if(strcmp(tempStr,"000300000001") == 0){
				sprintf(tempStr,"%02x%02x",arr[i+4][0],arr[i+4][1]);
				height=strtol(tempStr,&ptr,16);
			}
			if(strcmp(tempStr,"000400000001") == 0){
				sprintf(tempStr,"%02x%02x",arr[i+5][0],arr[i+5][1]);
				height=strtol(tempStr,&ptr,16);
			}
		}
		sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
		if(width!=-1 && height!=-1)
			break;
	}
	printf("Width : %ld pixels\nHeigt : %ld pixels\n",width,height);
	printf("Byte order : Motorola\n");

	/*pixel degerlerini 0 1 olarak tutmak için array allocate et width*height */
	pixels = (char*)calloc((width*height)+1,sizeof(char));
	for(i=0;i<width*height;i++)
		pixels[i]='2';
	pixels[width*height]='\0';

	/*eger pixeller 1er 1 olarak saklanıyorsa */
	if(bitControlType == 1 || bitControlType == 0){

		i=pixelStartIndex;/*pixel degerlerinin baslangıc indexi*/
		repeat=(width/16)+1; /*kac tane 16lık grup kullanılacak*/
		index=0;
		strcpy(tempStr2,"");
		for(j=0;j<height;j++){
			strcpy(tempStr2,"");
			for(k=0;k<repeat;k++){
				sprintf(tempStr,"%02x%02x",arr[i][0],arr[i][1]);
				/*sırası ile her 4 karakterin binary acılımını bul ve*/
				/*bir stringde sakla*/
				for(m=0;m<4;m++){
					strcat(tempStr2,hexToBinary(tempStr[m]));
				}
				i+=1;
			}
			/*her 16 lık grupta bir pixel değerlerini al saklanan stringden*/
			for(n=0;n<width;n++){
				pixels[index]=tempStr2[n];
				index+=1;
			}

		}
		
		/*print pixel results with 0's and 1's*/
		for(i=0;i<width*height;i++){
			if(i!=0 && i%(width) == 0)
				printf("\n");
			printf("%c",pixels[i]);
		}
		printf("\n");

	}

	/*eğer her pixel için 8 bit ayrılmış ve o sekilde saklanıyorsa*/
	if(bitControlType == 8){
		j=0;
		control=0;
		for(i=pixelStartIndex ; i<(width*height/2)+pixelStartIndex;i++){

			if(j!=0 && j%(width)==0){
				j=0;
				printf("\n");		
			}
			/*her ff değeri yani beyaz için 1 yazdır*/
			sprintf(tempStr,"%02x",arr[i][0]);	
			sprintf(tempStr2,"%02x",arr[i][1]);
			if(strcmp(tempStr,"ff") == 0){
				printf("1");
				j+=1;
			}
			/*her 00 değeri yani siyah için 0 yazdır*/
			if(strcmp(tempStr,"00") == 0){
				printf("0");
				j+=1;
			}					
			if(strcmp(tempStr2,"ff") == 0){
				if(j==width && width%2==1){
					printf("\n1");
					j=1;
				}
				else{
					printf("1");
					j+=1;
				}
			}
			if(strcmp(tempStr2,"00") == 0){
				if(j==width && width%2==1){
					printf("\n0");
					j=1;
				}
				else{
					printf("0");
					j+=1;
				}
			}	
			control+=1;
		}
		if(width%2 == 1){
			sprintf(tempStr,"%02x",arr[i][0]);	
			if(strcmp(tempStr,"ff") == 0)
				printf("1");
			if(strcmp(tempStr,"00") == 0)
				printf("0");
		}
		printf("\n");
	}



	/*deallocate the allocated memories*/
	for ( j = 0; j < len; j++ )
		free(arr[j]);
	free(arr);
	free(pixels);
}





/*function gets a character (in decimal) and returns it's binary form*/
const char * hexToBinary(char str){
	
	if(str=='0')
		return "0000";
	if(str=='1')
		return "0001";
	if(str=='2')
		return "0010";
	if(str=='3')
		return "0011";
	if(str=='4')
		return "0100";
	if(str=='5')
		return "0101";
	if(str=='6')
		return "0110";
	if(str=='7')
		return "0111";
	if(str=='8')
		return "1000";
	if(str=='9')
		return "1001";
	if(str=='a')
		return "1010";
	if(str=='b')
		return "1011";
	if(str=='c')
		return "1100";
	if(str=='d')
		return "1101";
	if(str=='e')
		return "1110";
	if(str=='f')
		return "1111";

	return "0000";
}














