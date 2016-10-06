/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

char* readFileBytes(const char *name)
{
	FILE *file = fopen(name, "r");
	if (file != NULL)
	{
		fseek(file, 0L, SEEK_END);
		long length = ftell(file);
		char *buffer = malloc(length);
		if(buffer != NULL)
		{			
			fseek(file, 0, SEEK_SET);
			fread(buffer, 1, length, file);
			//printf("O conteúdo do ficheiro é: %s\n", buffer);
			fclose(file);
			return buffer;
		}
	}
	printf("File is NULL.\n");
	return 0;
}

/*
to run the program
./writenoncanonical sender (left)/dev/ttyS0 receiver(right)/dev/ttyS1
*/

int main(int argc, char** argv)
{
    int fd, res;
    struct termios oldtio,newtio;

	char* cenas;

 
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

	cenas = readFileBytes(argv[2]);

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

/* save current port settings */
    if ( tcgetattr(fd,&oldtio) == -1) { 
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

//	char str[255];
	//printf("Write something: ");

//	gets(str);
//	scanf("%s",str);
/*	printf("\nYou Wrote : %s\n", str);
	size_t size = strlen(str);
	str[size+1] = '\0';*/

	size_t size = strlen(cenas);
	res = write(fd,cenas,size+1);
    

    
    printf("%d bytes written\n", res);
 
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
	sleep(2);
    close(fd);

    return 0;
}
