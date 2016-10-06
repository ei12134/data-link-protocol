#include <unistd.h>
#include <signal.h>
#include <stdio.h>

int flag=1, conta=1;

void atende()                   // atende alarme
{
	printf("alarme # %d\n", conta);
	flag=1;
	conta++;
}


int main(void)
{
    (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

    int i = 0;
    while(conta < 4){
        if (i% 10000000 == 0) {
            putchar('.');
            fflush(stdout);
        }
        if(flag){
            alarm(3);                 // activa alarme de 3s
            flag=0;
        }
        i++;
    }
    printf("Vou terminar.\n");
}

