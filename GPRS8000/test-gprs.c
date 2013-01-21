#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>	// open() close() 
#include <unistd.h>	// read() write()

#include <termios.h>	// set baud rate
#include <getopt.h>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define FUNC_RUN		0
#define FUNC_NOT_RUN		1

//#define SIMPLE_TEST 		1
#define READ_SIM_CARD_ID 	1
#define MAKE_A_CALL 		2
#define WAIT_A_CALL	 	3
#define SHORT_MESSAGE 		4
//#define POWER_MANAGE 		6
//#define GPRS_TEST		7
#define CMD			5
#define FUNC_QUIT 		6

#define SEND_SHORT_MESSAGE		1
#define READ_SHORT_MESSAGE		2
#define QUIT_SHORT_MESSAGE		0

#define MAX_LEN_OF_SHORT_MESSAGE	140

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

int speed_arr[] = {B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300,};
int name_arr[] = {230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300,};

#define RECEIVE_BUF_WAIT_1S 1
#define RECEIVE_BUF_WAIT_2S 2
#define RECEIVE_BUF_WAIT_3S 3
#define RECEIVE_BUF_WAIT_4S 4
#define RECEIVE_BUF_WAIT_5S 5
#define RECEIVE_BUF_WAIT_6S 6
#define RECEIVE_BUF_WAIT_8S 8

#define VERSION         "EMBEST_GPRS8000"

void showversion(void)
{
        printf("*********************************************\n");
        printf("\t %s \t\n", VERSION);
        printf("*********************************************\n\n");

}

//------------------------------------- read datas from GSM/GPRS --------------------------------------------
int read_GSM_GPRS_datas(int fd, char *rcv_buf,int rcv_wait)
{
	int retval;
	fd_set rfds;
	struct timeval tv;
	int ret,pos;

	pos = 0;

        tv.tv_sec = rcv_wait;
	tv.tv_usec = 0;

	while (1){
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		retval = select(fd+1 , &rfds, NULL, NULL, &tv);

		if (retval == -1)
		{
			perror("select()");
			return 0;
		}
		else if (0 == retval){
//			printf("time out\n");
			return 0;
//			break;
		}
		else {
			if(FD_ISSET(fd, &rfds)){
				ret = read(fd, rcv_buf+pos, 2048);
				pos += ret;
				if ((rcv_buf[pos-2] == '\r' && rcv_buf[pos-1] == '\n') 
						|| rcv_buf[pos-2] == '>') break;
			}
		}
	}
	return pos;
}

//------------------------------------- send cmd ------------------------------------------------------------
// succese return 1
// error   return 0
int send_GSM_GPRS_cmd(int fd, char *send_buf)
{
	ssize_t ret;
	
	ret = write(fd,send_buf,strlen(send_buf));
	if (ret == -1)
        {
                printf ("write device %s error\n", serial.serial_dev);
                return -1;
        }

	return 1;
} // end send_GSM_GPRS_cmd

//------------------------------------- send cmd and read back result ---------------------------------------
void GSM_GPRS_send_cmd_read_result(int fd, char *send_buf, int rcv_wait)
{
        char rcv_buf[2048];
	
	if((send_buf == NULL) || (send_GSM_GPRS_cmd(fd,send_buf)))
	{	// send success , then read
		bzero(rcv_buf,sizeof(rcv_buf));
		if (read_GSM_GPRS_datas(fd,rcv_buf,rcv_wait))
                {
                        printf ("%s\n",rcv_buf);
                }

	}
} // end GSM_GPRS_send_cmd_read_result

void checkstate(int fd)
{ 
        char *send_buf_normal="AT+CFUN=1\r";
	send_GSM_GPRS_cmd(fd,send_buf_normal);
	printf ("start GPRS8000 .....\n\n");
	sleep(2);
	//printf ("successful\n\n");
}

int GSM_GPRS_read_all_message(int fd, char *send_buf, char *rcv_buf, int rcv_wait){
	int ret = 0;
	if((send_buf == NULL) || (send_GSM_GPRS_cmd(fd,send_buf)))
	{
		bzero(rcv_buf,2048);
		if (ret = read_GSM_GPRS_datas(fd,rcv_buf,rcv_wait))
		{
			printf ("%s\n",rcv_buf);
			return ret;
		}
	} 
}

//------------------------------------- send cmd : "at" to GSM/GPRS MODEM -----------------------------------
void GSM_simple_test(int fd)
{	
	char *send_buf="at\r";
	
	GSM_GPRS_send_cmd_read_result(fd,send_buf,RECEIVE_BUF_WAIT_1S);

} // end GSM_simple_test

//------------------------------------- send cmd : "at+ccid" to GSM/GPRS MODEM ------------------------------
void GSM_read_sim_card_id(int fd)
{
        char *send_buf="AT+CCID\r";
	GSM_GPRS_send_cmd_read_result(fd,send_buf,RECEIVE_BUF_WAIT_1S);
	GSM_GPRS_send_cmd_read_result(fd,NULL,RECEIVE_BUF_WAIT_1S);
	sleep(2);
} // end GSM_read_sim_card_id

//------------------------------------- send cmd : "atd<tel_num>;" to GSM/GPRS MODEM ------------------------
//------------------------------------- finish call, send cmd:  "ath" to GSM MODEM --------------------------
void GSM_call(int fd)
{	
	char send_buf[17];
	char *send_cmd_ath = "ATH\r";
	char *send_cmd_at = "AT+CSQ\r";
	int i;

        // input telephone number
        bzero(send_buf,sizeof(send_buf));
	
        send_buf[0]='A';
        send_buf[1]='T';
        send_buf[2]='D';
	send_buf[16] = '\0';
			
	sleep(7);
	GSM_GPRS_send_cmd_read_result(fd,send_cmd_at,RECEIVE_BUF_WAIT_1S);//测试信号强度
	GSM_GPRS_send_cmd_read_result(fd,NULL,RECEIVE_BUF_WAIT_1S);
        
	printf("please input telephone number:");

        i = 3;
        while (1)
        {
                send_buf[i]=getchar();
                if (send_buf[i]=='\n') break;
                i++;
        }

        send_buf[i]=';';
        send_buf[i+1]='\r';
	// end input telphone number

	// send cmd
	GSM_GPRS_send_cmd_read_result(fd,send_buf,RECEIVE_BUF_WAIT_1S);//拨号

        //quit call
        printf("press ENTER for quit:");
        getchar();

        // send cmd
	GSM_GPRS_send_cmd_read_result(fd,send_cmd_ath,RECEIVE_BUF_WAIT_1S);//挂断
	
} // end GSM_call

//------------------------------------- wait for GSM/GPRS call signal ---------------------------------------
void GSM_wait_call(int fd)
{	
	char rcv_buf[2048];
	char *send_cmd_ath = "ATH\r";
	char *send_cmd_ata = "ATA\r";
//	char *RING = "RING\r\r\r";
        int wait_RING;
	int count=0;
        wait_RING = 300;
        while (wait_RING!=0)
        {	
		bzero(rcv_buf,sizeof(rcv_buf));

		if (read_GSM_GPRS_datas(fd,rcv_buf,RECEIVE_BUF_WAIT_1S))
                {
		     if(count)
		     {
                        printf ("%s\n",rcv_buf);
			GSM_GPRS_send_cmd_read_result(fd,send_cmd_ata,RECEIVE_BUF_WAIT_1S);
			printf("press ENTER for quit:");
			getchar();
			break;					
		     }
			count++;	
                }
		wait_RING--;
		sleep(1);
        }
	
	GSM_GPRS_send_cmd_read_result(fd,send_cmd_ath,RECEIVE_BUF_WAIT_1S);

        printf("quit wait_call\n");

} // end GSM_wait_call

//------------------------------------- GSM/GPRS Config short message env -----------------------------------
void GSM_Power_Manage(int fd)
{
	// 	char *send_buf_bat_charge="at+cbc\r";
	//	char *send_buf_min_fun="at+cfun=0\r";
		char *send_buf_dis_phone="at+cfun=4\r";
		char *send_buf_normal="at+cfun=1\r";
	//	GSM_GPRS_send_cmd_read_result(fd,send_buf_bat_charge,RECEIVE_BUF_WAIT_1S);
	//	GSM_GPRS_send_cmd_read_result(fd,send_buf_min_fun,RECEIVE_BUF_WAIT_1S);
		sleep(1);
		GSM_GPRS_send_cmd_read_result(fd,send_buf_dis_phone,RECEIVE_BUF_WAIT_1S);
		sleep(1);
		GSM_GPRS_send_cmd_read_result(fd,send_buf_normal,RECEIVE_BUF_WAIT_1S);

        printf("End Power Manage\n");
} // end GSM_Conf_Message

//------------------------------------- GSM/GPRS send short message -----------------------------------------
void GSM_Send_Message(int fd)
{	
        char cmd_buf[23];
        char short_message_buf[MAX_LEN_OF_SHORT_MESSAGE];
	int  i;
        char rcv_buf;

        bzero(cmd_buf,sizeof(cmd_buf));
	bzero(short_message_buf,sizeof(short_message_buf));

        printf ("send short message:\n");

        cmd_buf[0]='a';
        cmd_buf[1]='t';
        cmd_buf[2]='+';
        cmd_buf[3]='c';
        cmd_buf[4]='m';
        cmd_buf[5]='g';
        cmd_buf[6]='s';
        cmd_buf[7]='=';
        cmd_buf[8]='"';

        printf ("please input telephone number:");

        i = 9;
        while (1)
        {
                cmd_buf[i]=getchar();
                if (cmd_buf[i]=='\n') break;
                i++;
        }
	cmd_buf[i]='"';
        cmd_buf[i+1]='\r';
	cmd_buf[i+2]='\0';

        /* send telephone number */
	GSM_GPRS_send_cmd_read_result(fd,cmd_buf,RECEIVE_BUF_WAIT_1S);
	
        printf("please input short message:");

        i = 0;
        while(i < MAX_LEN_OF_SHORT_MESSAGE-2)
        {
                short_message_buf[i] = getchar();
                if (short_message_buf[i]=='\n') break;
                i++;
        }
        short_message_buf[i] = 0x1A;
        short_message_buf[i+1] = '\r';
	short_message_buf[i+2] = '\0';

        /* send short message */
	GSM_GPRS_send_cmd_read_result(fd, short_message_buf,RECEIVE_BUF_WAIT_4S);
	/* get more respond message */
	GSM_GPRS_send_cmd_read_result(fd, NULL, RECEIVE_BUF_WAIT_1S);

        printf("\nend send short message\n");
} // end GSM_Send_Message

//------------------------------------- GSM/GPRS read all short message -------------------------------------
void GSM_Read_Message(int fd)
{	
	int pos = 0;
	char *send_buf="at+cmgl=\"ALL\"\r";
	char rcv_buf[2048];

	/* 
	 * ?????????GSM_GPRS_read_all_message????????????????????GSM_GPRS_send_cmd_read_result????????????????????????
	 * ??????????????C??????????????????????????????????????????????????OK?????C???????????????????????? 
	 * */   
 	pos = GSM_GPRS_read_all_message(fd,send_buf,rcv_buf,RECEIVE_BUF_WAIT_1S);

       	while(rcv_buf[pos-4] != 'O' || rcv_buf[pos-3] != 'K'){	
		usleep(1000);
		pos = GSM_GPRS_read_all_message(fd,NULL,rcv_buf,RECEIVE_BUF_WAIT_1S);
	}

	printf("end read all short message\n");

} // end GSM_Read_Message

//------------------------------------- GSM/GPRS Config short message env -----------------------------------
void GSM_Conf_Message(int fd)
{	
	char *send_cmgf_buf="AT+CMGF=1\r";
	char *send_csmp_buf="at+csmp=17,0,0,0\r";
	char *send_center_buf="at+csca=\"+8613800755500\"\r";
	char *save_buf = "at&w\r";

	GSM_GPRS_send_cmd_read_result(fd,send_cmgf_buf,RECEIVE_BUF_WAIT_1S);
        GSM_GPRS_send_cmd_read_result(fd,send_csmp_buf,RECEIVE_BUF_WAIT_1S);
    	GSM_GPRS_send_cmd_read_result(fd,send_center_buf,RECEIVE_BUF_WAIT_1S);
	GSM_GPRS_send_cmd_read_result(fd, save_buf, RECEIVE_BUF_WAIT_1S);
        printf("end config short message env\n");
} // end GSM_Conf_Message


//------------------------------------- GSM/GPRS short message ----------------------------------------------
void GSM_short_mesg(int fd)
{	
        int flag_sm_run, flag_sm_select;

        flag_sm_run = FUNC_RUN;
	GSM_Conf_Message(fd);
        while (flag_sm_run == FUNC_RUN)
        {
                printf ("\n Select:\n");
                printf ("1 : Send short message \n");
                printf ("2 : Read all short message \n");
                printf ("0 : quit\n");
 
                printf (">");
                scanf("%d",&flag_sm_select);
                getchar();
		// temp
	//	printf ("input select:%d\n",flag_sm_select);
		// end temp
                switch (flag_sm_select)
                {
                        case SEND_SHORT_MESSAGE         :       { GSM_Send_Message(fd);         break; }
                        case READ_SHORT_MESSAGE         :       { GSM_Read_Message(fd);         break; }
                        case QUIT_SHORT_MESSAGE         :       { flag_sm_run = FUNC_NOT_RUN;	break; }
                        default :
                                {
                                        printf("please input your select use 1 to 3\n");
                                }
                }
        }
        printf ("\n");

} // end GSM_send_mesg

int GPRS_Net_Test(int fd)
{	
		
	char *cmd_buf1 = "at+cgreg?\r";
	char *cmd_buf2 = "at+cgact=1,1\r";
	char *cmd_buf3 = "at+cgdcont=1,\"IP\",\"cmnet\"\r";		
	char *cmd_buf4 = "at+cgatt=1\r";
	char *cmd_buf5 = "at+cipcsgp=1,\"cmnet\"\r";

	char cmd_buf[40];
       	char net_test_buf[50];
	char data[1024];
	int  i;
        char rcv_buf;
	int gprs_select;

        bzero(cmd_buf,40);
	bzero(net_test_buf, 50);
#if 0
	GSM_GPRS_send_cmd_read_result(fd,cmd_buf3,RECEIVE_BUF_WAIT_2S);	//the signal is ok?
	GSM_GPRS_send_cmd_read_result(fd,cmd_buf2,RECEIVE_BUF_WAIT_2S);	//active it!
	GSM_GPRS_send_cmd_read_result(fd,cmd_buf1,RECEIVE_BUF_WAIT_3S);	
#else
	GSM_GPRS_send_cmd_read_result(fd,cmd_buf4,RECEIVE_BUF_WAIT_1S);
	GSM_GPRS_send_cmd_read_result(fd,cmd_buf5,RECEIVE_BUF_WAIT_1S);
#endif
	printf("input the server ip:");
	i = 0;
	strcpy(cmd_buf, "at+cipstart=\"tcp\",\"");
	i = 19;

  	while (1)
        {
      		cmd_buf[i]=getchar();
         	if (cmd_buf[i]=='\n') break;
            	i++;
        }
	cmd_buf[i]='"';
	cmd_buf[++i] = ',';
	cmd_buf[++i] = '"';
	i++;

	printf("input the port:");
	while (1)
        {
      		cmd_buf[i]=getchar();
         	if (cmd_buf[i]=='\n') break;
            	i++;
        }
	cmd_buf[i] = '"';
      	cmd_buf[i+1]='\r';
	cmd_buf[i+2]='\0';	
	GSM_GPRS_send_cmd_read_result(fd,cmd_buf,RECEIVE_BUF_WAIT_1S);
	GSM_GPRS_send_cmd_read_result(fd,NULL,RECEIVE_BUF_WAIT_8S);

	while(1){
		printf("Select what you want to do:\n");
		printf("1:send data\n");
		printf("2:receive data\n");
		printf("0:exit\n");
		printf(">");

		scanf("%d",&gprs_select);
		getchar();

		switch(gprs_select){
			case 1:
				strcpy(cmd_buf, "at+cipsend\r");
				GSM_GPRS_send_cmd_read_result(fd,cmd_buf,RECEIVE_BUF_WAIT_2S);

				printf("send text:");
				i = 0;
				while (1)
        			{
                			data[i]=getchar();
					if (data[i] =='\n') break;
					i++;
					if (data[i] == 8) continue;
				}
				data[i]= 0x1a;
				data[++i] = '\r';
				data[++i] = '\0';
				GSM_GPRS_send_cmd_read_result(fd,data,RECEIVE_BUF_WAIT_4S);
				break;
			case 2:
				GSM_GPRS_send_cmd_read_result(fd,NULL,RECEIVE_BUF_WAIT_1S);
				break;
			case 0:
         			strcpy(cmd_buf, "at+cipclose\r");
         			GSM_GPRS_send_cmd_read_result(fd,cmd_buf,RECEIVE_BUF_WAIT_1S);
         			/* add by lfc */
	 			strcpy(cmd_buf, "at+cipshut\r");
         			GSM_GPRS_send_cmd_read_result(fd,cmd_buf,RECEIVE_BUF_WAIT_1S);
         			/* end add */
//				break;
				return 0;
			default:
				printf("please input your select use 1 to 3\n");
		}
	}
	return -1;
}

void CMD_TEST(int fd)
{	
	int i = 0;
	char cmd_buf[40];
	bzero(cmd_buf, sizeof(cmd_buf));

	printf("input the cmd:");
	while (1)
       	{
     	cmd_buf[i]=getchar();
      if (cmd_buf[i]=='\n') break;
		if (cmd_buf[i] ==	8) {
			printf("%c", 8);
			continue;
		}
      i++;
       	}
	cmd_buf[i] = '\r';
	cmd_buf[i+1] = '\0';
	GSM_GPRS_send_cmd_read_result(fd,cmd_buf,RECEIVE_BUF_WAIT_3S);
}
//------------------------------------- print ---------------------------------------------------------------
void print_prompt(void)
{	
        printf ("Select what you want to do:\n");
       // printf ("1 : Simple Test\n");
        printf ("1 : Read SIM CARD ID\n");
        printf ("2 : Make A Call\n");
        printf ("3 : Wait A Call\n");
        printf ("4 : Short message\n");
        //printf ("6 : Power manage\n");
        //printf ("7 : GPRS_TEST\n");	
	printf ("5 : self cmd\n");
        printf ("6 : QUIT\n");
        printf (">");
} // end print_prompt

//------------------------------------- Control GSM/GPRS MODULE ---------------------------------------------
void func_GSM(int fd)
{	
        int flag_func_run;
        int flag_select_func;
        ssize_t ret;

        flag_func_run = FUNC_RUN;

	/*get the message of "Call Ready" for the first time.*/
	GSM_GPRS_send_cmd_read_result(fd,NULL,RECEIVE_BUF_WAIT_1S);

        while (flag_func_run == FUNC_RUN)
        {
                print_prompt();			// print select functions
                scanf("%d",&flag_select_func);	// user input select
                getchar();

                switch(flag_select_func)
                {
                       // case SIMPLE_TEST        : {GSM_simple_test(fd);         break;}
                        case READ_SIM_CARD_ID   : {GSM_read_sim_card_id(fd);    break;}
                        case MAKE_A_CALL        : {GSM_call(fd);                break;}
                        case WAIT_A_CALL        : {GSM_wait_call(fd);           break;}
                        case SHORT_MESSAGE      : {GSM_short_mesg(fd);          break;}
                       // case POWER_MANAGE      	: {GSM_Power_Manage(fd);        break;}
                       //case GPRS_TEST		: {GPRS_Net_Test(fd);		break;}
			case CMD		: {CMD_TEST(fd); 		break;}		
                        case FUNC_QUIT          :
                                                {
                                                        flag_func_run = FUNC_NOT_RUN;
                                                        printf("Quit GSM/GPRS function.  byebye\n");
                                                        break;
                                                }
                        default :
                        {
                                printf("please input your select use 1 to 7\n");
                        }
                }

        }
}// end func_GPRS

//------------------------------------- init seriel port  ---------------------------------------------------
void init_ttyS(int fd, int i)
{	
        struct termios options;

        bzero(&options, sizeof(options));       // clear options

	switch (i)
        {
                case 230400 : options.c_cflag = (B230400 | CS8 | CLOCAL | CREAD);
                                break;
                case 115200 : options.c_cflag = (B115200 | CS8 | CLOCAL | CREAD);
                                break;
                case  57600 : options.c_cflag = (B57600 | CS8 | CLOCAL | CREAD);
                                break;
                case  38400 : options.c_cflag = (B38400 | CS8 | CLOCAL | CREAD);
                                break;
                case  19200 : options.c_cflag = (B19200 | CS8 | CLOCAL | CREAD);
                                break;
                case   9600 : options.c_cflag = (B9600 | CS8 | CLOCAL | CREAD);
                                break;
                case   4800 : options.c_cflag = (B4800 | CS8 | CLOCAL | CREAD);
                                break;
                case   2400 : options.c_cflag = (B2400 | CS8 | CLOCAL | CREAD);
                                break;
                case   1200 : options.c_cflag = (B1200 | CS8 | CLOCAL | CREAD);
                                break;
                case    300 : options.c_cflag = (B300 | CS8 | CLOCAL | CREAD);
                                break;
                default:
                        break;
        }

	options.c_iflag = IGNPAR;

	tcflush(fd, TCIFLUSH);

        tcsetattr(fd, TCSANOW, &options);
	checkstate(fd);
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
        for (i = 0; i < sizeof(speed_arr)/sizeof(int); i++)
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
                printf("\tYou must enter to open the device node!\n\n");
                print_usage(stderr, 1);
                exit (0);
        }

	strcpy(serial.serial_dev, dev);

        // open seriel port
        fd = open(serial.serial_dev, O_RDWR);

        if (fd == -1)
        {
                printf("open device %s error\n", serial.serial_dev);
        }
        else
        {
		speed_i = set_speed(serial.serial_speed);
	        showversion();
       		printf("\nGSM/GPRS TESTS\n\n");
		
		init_ttyS(fd, speed_i);	// init device
		func_GSM(fd);	// GSM/GPRS functions
                
                if (close(fd)!=0) printf("close device %s error", serial.serial_dev);
        }

        return 0;
}// end main




