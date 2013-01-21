// this is a test about GPS Receiver 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>      // open() close()
#include <unistd.h>     // read() write()

#include <termios.h>    // set baud rate
#include <getopt.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define FALSE   0
#define TRUE    1
#define WORDLEN 32

struct serial_config
{
        unsigned char serial_dev[WORDLEN];
        unsigned int  serial_speed;
        unsigned char databits;
        unsigned char stopbits;
        unsigned char parity;
};
struct serial_config serial;

int name_arr[] = {230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300,};


#define FUNC_RUN                0
#define FUNC_NOT_RUN            1

#define ORG_GPS		1
#define SEL_GPGGA	2
#define SEL_GPGLL	3
#define SEL_GPGSA	4
#define SEL_GPGSV	5
#define SEL_GPRMC	6
#define SEL_GPVTG	7
#define FUNC_QUIT	8

//------------------------------------- read datas from GPS -------------------------------------------------
// succese return 1
// error   return 0
int read_GPS_datas(int fd, char *rcv_buf)
{
	int retval;
        fd_set rfds;
        struct timeval tv;

        int ret,pos;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        pos = 0; // point to rceeive buf

        while (1)
        {
                FD_ZERO(&rfds);
                FD_SET(fd, &rfds);

                retval = select(fd+1 , &rfds, NULL, NULL, &tv);

                if (retval == -1)
                {
                        perror("select()");
                        break;
                }
                else if (retval)
                {// pan duan shi fou hai you shu ju
                        ret = read(fd, rcv_buf+pos, 2048);
                        pos += ret;
                        if (rcv_buf[pos-2] == '\r' && rcv_buf[pos-1] == '\n')
                        {
                                FD_ZERO(&rfds);
                                FD_SET(fd, &rfds);

                                retval = select(fd+1 , &rfds, NULL, NULL, &tv);

                                if (!retval) break;// if no datas, break
                        }
                }
                else
                {
                        printf("No data\n");
                        break;
                }
        }

        return 1;
} // end read_GPS_datas

//------------------------------------- FUNCTIONS ABOUT GPS -------------------------------------------------
//------------------------------------- FUNCTION 1 show all receive signal ----------------------------------
void GPS_original_signal(int fd)
{
	char rcv_buf[2048];

	while (1)
	{
		bzero(rcv_buf,sizeof(rcv_buf));
		{
			if (read_GPS_datas(fd,rcv_buf))
			{
				printf("%s",rcv_buf);
			}
		}
	}
} // end GPS_original_sign

//------------------------------------- init seriel port  ---------------------------------------------------
void init_ttyS(int fd, int i)
{
	struct termios newtio;
	tcgetattr(fd, &newtio);

	bzero(&newtio, sizeof(newtio));
	
	switch (i)
	{
		case 230400 : newtio.c_cflag = (B230400 | CS8 | CLOCAL | CREAD);
				break;
		case 115200 : newtio.c_cflag = (B115200 | CS8 | CLOCAL | CREAD);
                                break;
		case  57600 : newtio.c_cflag = (B57600 | CS8 | CLOCAL | CREAD);
                                break;
		case  38400 : newtio.c_cflag = (B38400 | CS8 | CLOCAL | CREAD);
                                break;
		case  19200 : newtio.c_cflag = (B19200 | CS8 | CLOCAL | CREAD);
                                break;
		case   9600 : newtio.c_cflag = (B9600 | CS8 | CLOCAL | CREAD);
                                break;
		case   4800 : newtio.c_cflag = (B4800 | CS8 | CLOCAL | CREAD);
                                break;
		case   2400 : newtio.c_cflag = (B2400 | CS8 | CLOCAL | CREAD);
                                break;
		case   1200 : newtio.c_cflag = (B1200 | CS8 | CLOCAL | CREAD);
                                break;
		case    300 : newtio.c_cflag = (B300 | CS8 | CLOCAL | CREAD);
                                break;
		default:
			break;
	}
	
	newtio.c_lflag &= ~(ECHO | ICANON);
	
	newtio.c_iflag = IGNPAR;//忽略接收到的数据的奇偶检验错误或帧错误（除了前面提到的中断条件）。
	newtio.c_oflag = 0;
	newtio.c_oflag &= ~(OPOST);//取消属性 不会对输出数据处理	

	newtio.c_cc[VTIME]    = 5;   /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 0;   /* blocking read until 9 chars received */
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);//设置新的串口属性

}//end init_ttyS

void print_usage (FILE *stream, int exit_code)
{
    fprintf(stream,
            "\t-h  --help     Display this usage information.\n"
            "\t-d  --device   The device ttyS[0-3] or ttyEXT[0-3]\n"
            "\t-b  --baudrate Set the baud rate you can select\n" 
	    "\t               [230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300]\n");

    exit(exit_code);
}

int set_speed(int speed)
{
	int i;
	int re_speed;
	for (i = 0; i < sizeof(name_arr)/sizeof(int); i++)
	{
		if(speed == name_arr[i])
		{
			re_speed = speed;
	        	break;
		}
	}
	
	if(i == 10)
	{
		printf("\tSorry, please set the correct baud rate!\n\n");
		print_usage(stderr, 1);
	}
	return re_speed;
}

//------------------------------------- main ----------------------------------------------------------------
int main(int argc, char *argv[])
{
	int fd;
	int opt;
	int speed_i;
	const char *dev = NULL;	
	const char *short_options= "hd:b:";
	struct option long_options[] =
	{
		{"help", 0, NULL, 'h'},
		{"device", 1, NULL, 'd'},
		{"baudrate", 1, NULL, 'b'},
		{0, 0, 0, 0},
	};

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
	{
		switch (opt)
		{
		case 'h':
			print_usage(stdout, 0);
		case 'd':
			dev = optarg;
			break;
		case 'b':
                        serial.serial_speed = atoi(optarg);
                        break;
                case '?':
                        print_usage(stderr, 1);
                default:
                        print_usage(stdout, 0);
		}
	}

        if (dev == NULL)
        {
                printf("\tYou must enter to open the device node\n\n");
                print_usage(stderr, 1);
                exit (0);
        }
        
	strcpy(serial.serial_dev, dev);

	// open seriel port
        fd = open(serial.serial_dev, O_RDONLY);

	if (fd == -1)
        {
                printf("open device %s error\n", serial.serial_dev);
        }
	
	speed_i = set_speed(serial.serial_speed);
	
	printf("\t\tctrl + c to exit!\n");
       	init_ttyS(fd, speed_i);  // init device
	GPS_original_signal(fd);
        
	
        if (close(fd)!=0) 
	{
		printf("close device %s error", (serial.serial_dev));
	}
	return 0;
 
} // end main
