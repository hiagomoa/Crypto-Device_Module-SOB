#include <iostream>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

void Decimal_em_Hexadecimal( char num );
int main()
{
    char aux;
    printf("Ola, Escolha uma Opcao: \n");


    printf("\n1- Para fazer a Criptaçao. Use c");
    printf("\n2- Para fazer a Descriptacao. Use d");
    printf("\n3- Para fazer Calculo de Rash. Use h\n");
    printf("\nOpcao:");
    scanf("%c", &aux);

    switch(aux){
        case 'c':
            char num;
            printf("Funcao cript\n");
            fflush(stdin);
            scanf("%c", &num);
            Decimal_em_Hexadecimal(num);

        break;

        case 'd':
            printf("Funcao descript");
        break;

        case 'h':
            printf("Funcao hash");
        break;
    }

    return 0;
}
void Decimal_em_Hexadecimal( char num[32] )
     {
	      int  i=0, j, loop,pos=0;
	      char  quociente;
          int resto[ i ];

          while(pos<32){
 	      quociente = num[pos] / 16;
 	      resto[ i ] = num[pos] % 16;

 	      while ( quociente >= 16 )
 	      {   i++;
 	          resto[ i ] = quociente % 16;
 	          quociente  = quociente / 16;
 	      }
 	      i++;
		  resto[ i ] = quociente;

          pos++;
          }
 	      printf ( "\nConversao Hexadecimal = ");
 	      for (  ; i >= 0; i-- )

		     if ( resto[ i ] >= 10 && resto[ i ]<=15 || resto[i]>=10){
                resto[ i ] = resto[ i ]+ 55;
                printf("%c", resto[i]);
		     }
			  else
                  printf ( "%d", resto[ i ] );

	 }

void convert_hexa( int vet[] )
    {   int vf;
        while(vf==0){

            vf=1
        }
    }



