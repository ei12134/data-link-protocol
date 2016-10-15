int llopen(char *port,int transmitter);
int llwrite(const int fd,const char *buffer,int length);
int llread(const int fd,const char *buffer,int buff_remaining);
int llclose(int fd);
