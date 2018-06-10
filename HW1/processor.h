#ifndef PROCESSOR_H
#define PROCESSOR_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*Fonksiyon file pointer input parametresini alır ve bu motorola dosyasınında*/
/*çeşitli taramalar yaparak dosyadaki pixellerin görüntüsünü 1 ve 0 larla 	 */
/*oluşturur . Motorola dosyalar içindir*/
void findMotorola(FILE *fptr);

/*Fonksiyon file pointer input parametresini alır ve bu intel dosyasınında*/
/*çeşitli taramalar yaparak dosyadaki pixellerin görüntüsünü 1 ve 0 larla */
/*oluşturur . Intel dosyalar içindir*/
void findIntel(FILE *fptr);

/*Fonksiyon bir karakter alır iinput olarak ve bu karakterin 4 bitlik binary*/
/*formunu string olarak return eder*/
const char * hexToBinary(char str);


#endif
