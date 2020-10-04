#include <stdio.h>
#include <stdlib.h>


void Decimal_em_Hexadecimal( char num[], char vet[] );
void convert_hexa( char vet[], char result[] );
int main()
{
    char aux;
    char num[30];
    char vet[60];
    char vet1[64],vet2[32];
    printf("Ola, Escolha uma Opcao: \n");


    printf("\n1- Para fazer a Criptaçao. Use c");
    printf("\n2- Para fazer a Descriptacao. Use d");
    printf("\n3- Para fazer Calculo de Rash. Use h\n");
    printf("\nOpcao:");
    scanf("%c", &aux);

    switch(aux)
    {
    case 'c':
        printf("Funcao cript\n");
        fflush(stdin);
        gets(num);
        Decimal_em_Hexadecimal(num,vet);
        break;

    case 'd':
        printf("Testando a funcao de hexa para decimal \n");
        fflush(stdin);
        gets(vet1);
        fflush(stdin);
        convert_hexa(vet1, vet2);

        break;

    case 'h':
        printf("Funcao hash");
        break;
    }

    return 0;
}
void Decimal_em_Hexadecimal( char num[], char vet[] )
{
    int loop=0,pos=0;

    while(pos<2)
    {

        vet[pos]= (num[loop]/16);

        printf("\nO valor em hexadecimal eh: ");
        if(vet[pos]>9)
        {
            vet[pos]+=87;
            printf("%c", vet[pos]);
            pos++;
        }
        else
        {
            vet[pos]+=48;
            printf("%d", vet[pos]-48);
            pos++;
        }
        //------------------------------------------------------------------------------------

        vet[pos]= (num[loop]%16);
        if(vet[pos]>9)
        {
            vet[pos]+=87;
            printf("%c", vet[pos]);
            pos++;
        }
        else
        {
            vet[pos]+=48;
             printf("%d", vet[pos]-48);
            pos++;

        }

        loop++;
    }

}


void convert_hexa( char vet[], char result[] )
{
    int pos=0,i=0,valor=0;
    while(pos<32)
    {

        if(vet[pos]>=97 && vet[pos]<=122)
        {
            valor+=(vet[pos]-87)*16;
        }

        else if(vet[pos]>=65 && vet[pos]<=90)
        {
            valor+=(vet[pos]-55)*16;
        }
        else
        {
            valor+=(vet[pos]-48)*16;
        }
        //-----------------------------------------------------------------------------------------------------------------------------
        if(vet[pos+1]>=97 && vet[pos+1]<=122)
        {
            valor+=(vet[pos+1]-87);
        }
        else if(vet[pos+1]>=65 && vet[pos+1]<=90)
        {
            valor+=(vet[pos+1]-55);
        }
        else
        {
            valor+=(vet[pos+1]-48);
        }
        result[i]=valor;
        valor=0;
        i=i+1;
        pos=pos+2;

    }
    result[i]='\0';
    puts(result);
}







