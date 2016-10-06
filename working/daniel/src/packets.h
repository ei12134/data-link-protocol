int llopen(char *port,int transmitter);
int llwrite(int fd,char *buffer,int length);
int llread(int fd,char *buffer,int buff_remaining);
int llclose(int fd);
