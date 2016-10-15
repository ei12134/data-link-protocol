#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

typedef unsigned char byte;

//#define BAUDRATE B38400
#define BAUDRATE B300
#define _POSIX_SOURCE 1 /* POSIX compliant source */

struct termios g_oldtio;

byte g_previous_last_byte = 0;
byte serial_port_previous_last_byte() { return g_previous_last_byte; }

byte g_last_byte = 0;
byte serial_port_last_byte() { return g_last_byte; }

int serial_port_open(const char *dev_name,const int micro_timeout)
{
    #ifdef SERIAL_PORT_DEBUG_MODE
    fprintf(stderr,"serial_port_open(): entering function; dev = %s\n, timeout = \
            %d\n",dev_name,micro_timeout);
    #endif

    /*
       Open serial port device for reading and writing and not as controlling
       tty because we don't want to get killed if linenoise sends CTRL-C.
       */
    int fd = -1;
    struct termios newtio;

    fd = open(dev_name, O_RDWR | O_NOCTTY);
    if (fd < 0) {
	    perror(dev_name);
	    return -1;
    }

    #ifdef SERIAL_PORT_DEBUG_MODE
    fprintf(stderr,"isatty()=%d, ttyname()=%s\n",isatty(fd),ttyname(fd));
    fprintf(stderr,"fd = %d\n",fd);
    #endif

    /* Port settings */
    if (tcgetattr(fd,&g_oldtio) == -1) { /* save current port settings */
	    perror("tcgetattr");
	    return -1;
    }
    bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    //newtio.c_iflag = IGNPAR | ICRNL;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;
    newtio.c_oflag &= ~OCRNL;

    /* set input mode (non-canonical, no echo,...) */
    //newtio.c_lflag &= ~(ICANON | ECHO);
    newtio.c_lflag = 0;

    /* 0 => inter-character timer unused */
    newtio.c_cc[VTIME] = micro_timeout;
	
    /* VMIN=1 => blocking read until 1 character is received */   
    //newtio.c_cc[VMIN] = (micro_timeout == 0) ? 1 : 0;
    newtio.c_cc[VMIN] = 1;

    #ifdef SERIAL_PORT_DEBUG_MODE
    fprintf(stderr,"serial_port_open(): timeout=%d, c_cc[VMIN]=%d\n",micro_timeout,
            newtio.c_cc[VMIN]);
    #endif

    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    return fd;
}

int serial_port_close(int fd,int close_wait_time)
{
    #ifdef SERIAL_PORT_DEBUG_MODE
    fprintf(stderr,"serial_port_close(): waiting %d seconds to close...\n",
            close_wait_time);
    #endif
    sleep(close_wait_time);

    int ret = tcsetattr(fd, TCSANOW, &g_oldtio);
    if (ret == -1) {
        perror("tcsetattr");
        return -1;
    }
    return close(fd);
}

int serial_port_write(int fd,byte *data,int len)
{
    #ifdef SERIAL_PORT_DEBUG_MODE
    fprintf(stderr,"serial_port_write(): writting: length = %d, fd = %d\n",len,fd);
    #endif

    int result = write(fd,data,len);

    if (result < 0) {
        fprintf(stderr,"serial_port_write(): error %d, errno = %x\n",result,
                errno);
    #ifdef SERIAL_PORT_DEBUG_MODE
    } else {
        fprintf(stderr,"serial_port_write(): wrote %d bytes\n",result);
    #endif
    }

    return result;
}

/** \brief Read from serial port until either:
 * - a delimiter char is found
 * - the maximum number of chars is read 
 * - there is a timeout
 *
 * @return Number of chars read or negative number if error
 * */
int serial_port_read(int fd,byte *data,byte delim,int maxc)
{
    #ifdef SERIAL_PORT_DEBUG_MODE
    fprintf(stderr,"serial_port_read(): entering function\n");
    fprintf(stderr,"                    delimiter = %x\n",(char)delim);
    #endif

    g_previous_last_byte = g_last_byte;

    byte *p = data;
    int nc = 0; // num chars read so far
    do {
        int ret = read(fd,&g_last_byte,1);
        if (ret == 0) {
            #ifdef SERIAL_PORT_DEBUG_MODE
            fprintf(stderr,"serial_port_read(): micro timeout tick\n");
            #endif
            break;
        }
	else if (ret < 0 && errno == EINTR) { // interrupted, possibly by an alarm
            #ifdef SERIAL_PORT_DEBUG_MODE
            fprintf(stderr,"serial_port_read(): received interrupt\n");
            #endif
	    return 0;
        }
	else if (ret < 0) {
            #ifdef SERIAL_PORT_DEBUG_MODE
            fprintf(stderr,"serial_port_read(): ret = %d, errno = %d\n",ret,errno);
            #endif
	}
        #ifdef SERIAL_PORT_PRINT_ALL_CHARS
        fprintf(stderr,"< %x\n",g_last_byte);
        //fprintf(stderr,"c: %d/%c, nc: %d, ret: %d\n",c,c,nc,ret);
        #endif

        *p++ = g_last_byte;
        nc++;
    } while (nc < maxc && g_last_byte != delim);

    #ifdef SERIAL_PORT_DEBUG_MODE
    //fprintf(stderr,"serial_port_read(): read (%d): %.*s\n",nc,nc,data);
    #endif

    return nc;
}

