/**
 * @file   testebbcharmutex.c
 * @author Derek Molloy
 * @date   7 April 2015
 * @version 0.1
 * @brief  A Linux user space program that communicates with the ebbchar.c LKM. It passes a
 * string to the LKM and reads the response from the LKM. For this example to work the device
 * must be called /dev/ebbchar.
 * @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
*/
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include <stdio.h>
#include <string.h>



#define BUFFER_LENGTH 16               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM


void convert_hexa(char* input, char* output){
   int loop=0;
   int i=0;
   while(input[loop] != '\0'){
      sprintf((char*)(output+i),"%02X", input[loop]);
      loop+=1;
      i+=2;
   }
   //marking the end of the string
   output[i++] = '\0';
}


static void preencher(char* str, int tam){
printf("valor do tamanho %d \n", tam);	
int i=0;
	for(i=tam; i<16; i++){
	  str[i]='0';
   }
	str[16]='\0';

}

int main(){
   int ret, fd;
   char stringToSend[BUFFER_LENGTH];
char valor[32];
char convertido[33];
char nova[33];
   //printf("Starting device test code example...\n");
   fd = open("/dev/ebbchar", O_RDWR);             // Open the device with read/write access
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }
   //printf("Type in a short string to send to the kernel module:\n");
    printf("Ola, Escolha uma Opcao: \n");
    printf("\n1- Para fazer a Criptaçao.");
    printf("\n2- Para fazer a Descriptacao.");
    printf("\n3- Para fazer Calculo de Hash.\n");
    printf("\nOpcao:");
   scanf("%[^\n]%*c", stringToSend);              // Read in a string (with spaces)
   printf("Writing message to the device [%s].\n", stringToSend);

if(stringToSend[0] == 'e'){
	int a=0;
	for(int i=2;i<=strlen(stringToSend); i++){
	valor[a]=stringToSend[i];
	a++;
	}

	
	preencher(valor, strlen(valor));

	printf("ASSSSSSSS: %s------", valor);

	int i=0 ,j = 0;
	convert_hexa(valor, convertido);
	printf("\n Valor em hexa: %s---------\n", convertido);
	sprintf(nova,"%c %s", stringToSend[0], convertido);
	printf("\n TUUUUUDDOOOOOOOO: %s---------\n", nova);
	
   ret = write(fd, nova, strlen(convertido)); // Send the string to the LKM
}else{
   ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
}
   if (ret < 0){
      perror("Failed to write the message to the device.");
      return errno;
   }

   printf("Press ENTER to read back from the device...");
   getchar();

   printf("Reading from the device...\n");
   ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   printf("The received message is: [%s]\n", receive);
   printf("End of the program\n");
   return 0;
}
