/*
 ============================================================================
 Name        : main.c
 Author      : winston
 Version     :
 Copyright   : Your copyright notice
  ============================================================================
 */







//#include <stdio.h>
//#include <stdlib.h>
#include <stdlib.h> 
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include "./lib/parser/ini.h"
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

#define openPath		"/dev/ttyDOOR"
#define buadrate		19200

#define STX_VALUE 		0xF7

typedef struct
{
	char STX;
	char DID;
	char SID;
	char CMD;
	char D_LEN[2];
	char DATA[8];
	char XORSUM;
	char ADDSUM;
} SendFormat;

SendFormat sendBuf;

typedef struct
{
	char STX;
	char DID;
	char SID;
	char CMD;
	char D_LEN[2];
	char DATA[8];
	char XORSUM;
	char ADDSUM;
} ReceiveFormat;

ReceiveFormat receiveBuf;
SendFormat *p_sendBuf = &sendBuf;

typedef struct
{
//	const char* useRelay0;
//	const char* useRelay1;
//	int grantAccessTime0;
//	int grantAccessTime1;
//	int tofmin;
//	int tofmax;
//	int verifyVolume;
//	int tamperEnable;
//	int activeType;
	char* modelType;
} Configuration;


static int handler(void* config, const char* section, const char* name, const char* value)
{
	Configuration* pconfig = (Configuration*)config;

//	if (MATCH("relay", "useRelay0")) {
//		pconfig->useRelay0 = strdup(value);
//	} else if (MATCH("relay", "useRelay1")) {
//		pconfig->useRelay1 = strdup(value);
//	} else if (MATCH("relay", "grantAccessTime0")) {
//		pconfig->grantAccessTime0 = atoi(value);
//	} else if (MATCH("relay", "grantAccessTime1")) {
//		pconfig->grantAccessTime1 = atoi(value);
//	} else if (MATCH("system", "tofmin")) {
//		pconfig->tofmin= atoi(value);
//	} else if (MATCH("system", "tofmax")) {
//		pconfig->tofmax= atoi(value);
//	}
//	else if (MATCH("uart", "verifyVolume")) {
//			pconfig->verifyVolume = atoi(value);
//	}
//	else if (MATCH("uart", "tamperEnable")) {
//			pconfig->tamperEnable = atoi(value);
//	}
//	else if (MATCH("uart", "activeType")) {
//			pconfig->activeType = atoi(value);
//	}
//	else if (MATCH("uart", "modelType")) {
//			pconfig->modelType = strdup(value);
//	}

	if (MATCH("uart", "modelType"))
	{
				pconfig->modelType = strdup(value);
	}

	else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}


Configuration gConfig;

pthread_t threads[3];
redisContext *event;
static int fd; // Serial Port File Descriptor
int rdcnt;


void *thread_function_redis_on_Verify(void *);
void *thread_function_receive_uart(void *);
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);

void sendMsg(SendFormat *sendBuf, char cmd, char data);

int strUpr(char *str)
{

	int loop = 0;
	while( str[loop] != '\0' )
	{
		str[loop] = (char) toupper( str[loop] );
		loop++;
	}
	return loop;
}

redisContext* redisInitialize() {

    redisContext* c;
    const char *hostname = "127.0.0.1";
    int port = 6379;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds

    c = redisConnectWithTimeout(hostname, port, timeout);

    if (c == NULL || c->err) {
	}

    return c;

}
void *thread_function_redis_on_Verify(void *arg)
{
	struct event_base *base = event_base_new();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err)
	{
		pthread_exit((void *) 0);
	}

	redisLibeventAttach(c, base);

	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE Verify");

	event_base_dispatch(base);


	pthread_exit((void *) 0);
}
void *thread_function_receive_uart(void *arg)
{
	char buf[2];
	char serialBuf[2];
	int bufCnt = 0;
	int i;
	memset(serialBuf, 0, 2);


	while(1)
	{//start while(1)
		rdcnt = read(fd, buf, sizeof(buf));
		if (rdcnt > 0)//start if((rdcnt > 0))
		{
			for(i=0; i<rdcnt; i++)
			{
				serialBuf[i] = buf[i];
				bufCnt++;
			}

			printf("receive  <-- %s \n\r", serialBuf);

			memset(buf, 0, 2);
			memset(serialBuf, 0, 2);
		}
	}
}
static void onMessage(redisAsyncContext *c, void *reply, void *privdata)
{

	int i, j, rtrn, len;
	redisReply *r = reply;

	char str[1024];
	char *ptr;
	if (reply == NULL)
		return;

	if (r->type == REDIS_REPLY_ARRAY)
	{
		for (j = 0; j < r->elements; j++)
		{
//			printf("%u) %s\n", j, r->element[j]->str);

		}

		if (strcasecmp(r->element[0]->str, "message") == 0	&& strcasecmp(r->element[1]->str, "Verify") == 0 )
		{
			strcpy(str, r->element[2]->str);
			printf("str : %s \n\r ", str);

			ptr = strtok(str, ";");
			if( (strcasecmp(ptr, "start") == 0) )
			{//case "start;"
				if( (strcasecmp(gConfig.modelType, "solo") == 0) || (strcasecmp(gConfig.modelType, "suite") == 0) )
				{

				}
				else
				{

				}

			}
			else if ( (strcasecmp(ptr, "stop") == 0) )
			{//case "stop;"
				if( (strcasecmp(gConfig.modelType, "solo") == 0) || (strcasecmp(gConfig.modelType, "suite") == 0) )
				{

				}
				else
				{

				}

			}
			else if ( (strcasecmp(ptr, "result") == 0) )
			{
				if(ptr != NULL)
				{
					ptr = strtok(NULL, ";");
					if( (strcasecmp(ptr, "3") == 0) )		//palm
					{
						if(ptr != NULL)
						{
							ptr = strtok(NULL, ";");
							if( (strcasecmp(ptr, "0") == 0) )		//fail
							{

							}
							else if ( (strcasecmp(ptr, "1") == 0) )	//success
							{

							}
							else
							{

							}
						}
					}

					else if( (strcasecmp(ptr, "4") == 0) )		//card
					{
						if(ptr != NULL)
						{
							ptr = strtok(NULL, ";");
							if( (strcasecmp(ptr, "0") == 0) )		//fail
							{

							}
							else if ( (strcasecmp(ptr, "1") == 0) )	//success
							{

							}
							else
							{

							}
						}
					}

					else { }//not todo
				}
				else{ }//not todo
			}
			else if( (strcasecmp(ptr, "photo") == 0) )
			{
				if( (strcasecmp(gConfig.modelType, "solo") == 0) || (strcasecmp(gConfig.modelType, "suite") == 0) )
				{
					ptr = strtok( NULL, ";");
					if( ptr == NULL ) return;
					//type don't used

					ptr = strtok( NULL, ";");
					if( ptr == NULL ) return;

					ptr = strtok( NULL, ";");
					if( ptr == NULL ) return;


					if( strcasecmp(gConfig.modelType, "suite") == 0 )
					{
						printf("send open door. cmd = 0x41, palm data value = 0x01  \n\r");
						sendMsg(p_sendBuf, 0x41, 0x01);
					}
				}
				else
				{
					//don't define
				}

			}

			else if( (strcasecmp(ptr, "fail") == 0) )
			{

			}
			else if( (strcasecmp(ptr, "palmstart") == 0) )
			{

			}
			else if( (strcasecmp(ptr, "palmstop") == 0) )
			{

			}
			else {  }//not todo
		}
		else{  } //not todo
	}
}

int  open_serial( char *dev_name, int baud, int vtime, int vmin )
{
	int           fd;
	struct  termios    newtio;

	fd = open( dev_name, O_RDWR | O_NOCTTY );
	if ( fd < 0 )
	{
		printf(  "Device  OPEN  FAIL  %s\n",  dev_name  );
		return -1;
	}
	memset(&newtio,  0,  sizeof(newtio));
	newtio.c_iflag  =  IGNPAR;
	//  non-parity
	newtio.c_oflag  =  0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD;  // NO-rts/cts
	switch( baud )
	{
		case 115200 	: newtio.c_cflag 	|= B115200; 	break;
		case 57600    	: newtio.c_cflag 	|= B57600;    	break;
		case 38400    	: newtio.c_cflag 	|= B38400;    	break;
		case 19200    	: newtio.c_cflag 	|= B19200;    	break;
		case  9600      :  newtio.c_cflag  	|=  B9600;      break;
		case  4800      :  newtio.c_cflag  	|=  B4800;      break;
		case  2400      :  newtio.c_cflag  	|=  B2400;      break;
		default         :  newtio.c_cflag  	|=  B115200;  	break;
	}
	//set input mode (non-canonical, no echo,.....)
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME]  =  vtime;    //  timeout  0.1
	newtio.c_cc[VMIN]    =  vmin;      //
	tcflush    (  fd,  TCIFLUSH  );
	tcsetattr(  fd,  TCSANOW,  &newtio  );

	/*---------------------------------------- Controlling RTS and DTR Pins --------------------------------------*/
	int DTR_flag;
	DTR_flag = TIOCM_DTR;	/* Modem Constant for DTR pin */
	ioctl(fd,TIOCMBIS,&DTR_flag); 		/* setting DTR = 1,~DTR = 0 */
	ioctl(fd,TIOCMBIC,&DTR_flag); 		/* setting DTR = 0,~DTR = 1 */
	/*---------------------------------------- Controlling RTS and DTR Pins --------------------------------------*/

	return  fd;
}

	void
close_serial( int fd )
{
	close( fd );
}

int main(int argc, char **argv)
{


	printf("serial initialize \n\r");
	printf("open path = %s, buadrate = %d \n\r", openPath, buadrate);
	fd = open_serial( openPath, buadrate, 10, 32 );
	if(fd == -1)
	{
		return 0;
	}







	//redis thread create ( Verify )
	if (pthread_create(&threads[0], NULL, &thread_function_redis_on_Verify, NULL) == -1)	//verify
	{

	}

	//thread fuction for receive message from jj
	if (pthread_create(&threads[1], NULL, &thread_function_receive_uart, NULL) == -1)
	{

	}

	event = redisInitialize();

	memset( &gConfig, 0, sizeof( Configuration ) );
	if (ini_parse("settings.ini", handler, &gConfig) < 0) {
		printf("Can't load 'settings.ini'\n");
		return 1;
	}
	printf("Config load from 'settings.ini': modelType : %s \n\r", 	gConfig.modelType);



    while(1)
    {//start while

    	usleep(1000 * 100);	//100ms while process

    }//end while
    close(fd);

    return 0;
}

void sendMsg(SendFormat *sendBuf, char cmd, char data)
{
	printf(" sizeof(sendBuf)        	= %d \n\r",  sizeof(SendFormat));
	memset(sendBuf, 0, sizeof(SendFormat));


	sendBuf->STX = STX_VALUE;
	sendBuf->DID = 0x31;
	sendBuf->SID = 0x00;
	sendBuf->CMD = cmd;
	sendBuf->D_LEN[0] = 0x00;
	sendBuf->D_LEN[1] = 0x08;
	sendBuf->DATA[0] = data;
	sendBuf->XORSUM = sendBuf->STX ^ sendBuf->DID ^ sendBuf->SID ^ sendBuf->CMD ^ sendBuf->D_LEN[0] ^ sendBuf->D_LEN[1] ^ sendBuf->DATA[0];
	sendBuf->ADDSUM = sendBuf->STX + sendBuf->DID + sendBuf->SID + sendBuf->CMD + sendBuf->D_LEN[0] + sendBuf->D_LEN[1] + sendBuf->DATA[0] + sendBuf->XORSUM;



	printf(" \n\r");

//	printf(" sendBuf->STX		  = 0x%02X \n\r",  sendBuf->STX);
//	printf(" sendBuf->DID		  = 0x%02X \n\r",  sendBuf->DID);
//	printf(" sendBuf->SID		  = 0x%02X \n\r",  sendBuf->SID);
//	printf(" sendBuf->CMD		  = 0x%02X \n\r",  sendBuf->CMD);
//	printf(" sendBuf->D_LEN[0]	  = 0x%02X \n\r",  sendBuf->D_LEN[0]);
//	printf(" sendBuf->D_LEN[1]	  = 0x%02X \n\r",  sendBuf->D_LEN[1]);
//	printf(" sendBuf->DATA[0]	  = 0x%02X \n\r",  sendBuf->DATA[0]);
//	printf(" sendBuf->XORSUM	  = 0x%02X \n\r",  sendBuf->XORSUM);
//	printf(" sendBuf->ADDSUM	  = 0x%02X \n\r",  sendBuf->ADDSUM);

	int rtn = write(fd, sendBuf, sizeof(SendFormat));

}
