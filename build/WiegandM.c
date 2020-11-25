
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <termio.h>
#include <pthread.h>

#include <sys/wait.h>
#include <sys/stat.h> 
#include <sys/signal.h> 
#include <sys/ioctl.h> 
#include <sys/poll.h>   
#include <termios.h> 
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>

#include <wiringPi.h>


//#include <gpio.h>

#define LOGPATH				"/work/log/WiegandM"
#define NAME				"Wiegand"

//#include "gpio_control.h"
#include "./lib/parser/ini.h"


#define SQLITE_SAFE_FREE(x)	if(x){ x = NULL; }
#define BUFFER_SIZE 	2048

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

//wiringPi port
#define D0	11
#define D1	6

char gpio_key[20];
char gpio_value[20];

void wiringPi_io_setup(void)
{
	int ret = wiringPiSetup();
	if(ret == 0)	printf("wiringPi init success \n\r");


	pinMode(D0, 	OUTPUT);
	pinMode(D1, 	OUTPUT);

	digitalWrite(D0, 	HIGH);
	digitalWrite(D1,	HIGH);

}


typedef struct
{
	int facilityCode;
	int cardType;
	int pulseWidth;
	int interPulseGap;
} Configuration;

static int handler(void* config, const char* section, const char* name, const char* value)
{
	Configuration* pconfig = (Configuration*)config;

	if (MATCH("wiegand", "facilityCode")) {
		pconfig->facilityCode = atoi(value);
	} else if (MATCH("wiegand", "pulseWidth")) {
		pconfig->pulseWidth = atoi(value);
	} else if (MATCH("wiegand", "interPulseGap")) {
		pconfig->interPulseGap = atoi(value);

	} else if (MATCH("wiegand", "cardType")) {
		pconfig->cardType = atoi(value);
	} else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}


void Card_26Bit_Format();
void Card_34Bit_Format();
void Card_35Bit_Format();
void sig_handler(int signo);

void *thread_wiegand_out(void *arg);
void *redis_GPIO_REV(void *arg);
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);

void outD0(void);
void outD1(void);
Configuration gConfig;
pthread_t threads[5];

int d0fd;
int d1fd;


static int fd; // Serial Port File Descriptor
static int iconfd; // Serial Port File Descriptor

static redisContext *event;
//static CONFIGINFO configInfo;

int running = 1;

int eventCounter = 0;
const int MaxBitCount = 64;
int gbWgInCur = 0;
char wgbuf[64];

void inD0Int(void) {


	//if( gbWgInCur > 68 ) gbWgInCur = 0;
	if( gbWgInCur > 70 ) gbWgInCur = 0;

	wgbuf[gbWgInCur++] = 0;


}

void inD1Int(void) {


	//if( gbWgInCur > 68 ) gbWgInCur = 0;
	if( gbWgInCur > 70 ) gbWgInCur = 0;

	wgbuf[gbWgInCur++] = 1;

}

void outD0(void)
{

	digitalWrite(D0, 	LOW);

//	write(d0fd, "0", 1);
	//write(d0fd, "1", 1); 
	usleep(gConfig.pulseWidth);	// add 170904 for ini parser

	digitalWrite(D0, 	HIGH);
//	write(d0fd, "1", 1);
	//write(d0fd, "0", 1); 

	printf("0");
}
void outD1(void)
{
	digitalWrite(D1, 	LOW);
//	write(d1fd, "0", 1);
	//write(d1fd, "1", 1); 
	usleep(gConfig.pulseWidth);	// add 170904 for ini parser

	digitalWrite(D1, 	HIGH);
//	write(d1fd, "1", 1);
	//write(d1fd, "0", 1); 

	printf("1");
}

redisContext* redisInitialize() {

	redisContext* c;
	const char *hostname = "127.0.0.1";
	//const char *hostname = "192.168.100.110";
	int port = 6379;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds

	c = redisConnectWithTimeout(hostname, port, timeout);

	if (c == NULL || c->err) {
	}
	/*
	   else
	   debugPrintf("1", "Connection\n");


	   if (c == NULL || c->err) {

	   writeLogV2("/work/smart/log/", "[SetupManager]", "DB Fail" );

	   if (c) {
	   debugPrintf(configInfo.debug, "Connection error: %s\n", c->errstr);
	   redisFree(c);
	   } else {
	   debugPrintf(configInfo.debug, "Connection error: can't allocate redis context\n");
	   }
	   exit(1);
	   }
	   else
	   writeLogV2("/work/smart/log/", "[SetupManager]", "DB OK" );
	 */

	return c;

}


//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------ 
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
		case 115200 : newtio.c_cflag |= B115200; break; 
		case 57600    : newtio.c_cflag |= B57600;    break; 
		case 38400    : newtio.c_cflag |= B38400;    break; 
		case 19200    : newtio.c_cflag |= B19200;    break; 
		case  9600      :  newtio.c_cflag  |=  B9600;      break;  
		case  4800      :  newtio.c_cflag  |=  B4800;      break;  
		case  2400      :  newtio.c_cflag  |=  B2400;      break;  
		default          :  newtio.c_cflag  |=  B115200;  break;  
	}       
	//set input mode (non-canonical, no echo,.....) 
	newtio.c_lflag = 0; 
	newtio.c_cc[VTIME]  =  vtime;    //  timeout  0.1
	newtio.c_cc[VMIN]    =  vmin;      //  
	tcflush    (  fd,  TCIFLUSH  );  
	tcsetattr(  fd,  TCSANOW,  &newtio  );  
	return  fd;  
} 
//------------------------------------------------------------------------------ 
//------------------------------------------------------------------------------

void close_serial( int fd ) 
{ 
	close( fd ); 
} 

int main(int argc, char **argv)
{

	int i, led;
	pthread_t p_thread;
	int thr_id;
	int status;

	int rc;

	void *socket_fd;
	struct sockaddr_in servaddr; //server addr

	// wifi
	struct termios tio, old_tio;
	int ret;

	unsigned char DataBuf[BUFFER_SIZE];
	int ReadMsgSize;

	unsigned char receiveBuffer[BUFFER_SIZE];
	int receiveSize = 0;
	unsigned char remainder[BUFFER_SIZE];
	int parsingSize = 0;

	char th_data[256];

	int dataToggle = 0;

	memset( &gConfig, 0, sizeof( Configuration ) );	// add 170904 for ini parser
	memset( th_data, 0, sizeof( th_data ) );
	memset( wgbuf, 0, MaxBitCount);

	signal( SIGINT, (void *) sig_handler);

	if (ini_parse("settings.ini", handler, &gConfig) < 0) {
		printf("Can't load 'settings.ini'\n");
		return 1;
	}
	printf("Config loaded from 'settings.ini': cardType=%d, facilityCode=%d, pulseWidth=%d, interPulseGap=%d\n",
			gConfig.cardType, gConfig.facilityCode, gConfig.pulseWidth, gConfig.interPulseGap);

	//bacsNum= atoi(argv[1]);	// remove 170904 : Do not use any more


	//init gpio
	printf("init gpio\n");
	wiringPi_io_setup();

	//with out wiringpi
	/*
	gpio_open_port(15);		//pin open
	gpio_open_port(16);		//pin open
	gpio_init_port(15, 0);	//pin output set
	gpio_init_port(16, 0);	//pin output set
	wiringPi_io_setup();

	char buf[128];
	int gpio = 425; 
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	d0fd = open(buf, O_WRONLY);
	write(d0fd, "1", 1); // set high

	gpio = 424;
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	d1fd = open(buf, O_WRONLY);
	write(d1fd, "1", 1); // set high
	*/

	event = redisInitialize();
	//fd = open_serial( devName, atoi(xmlinfo.comport.baud), 10, 32 ); 
//	fd = open_serial( "/dev/ttyPANTILT", 9600, 10, 32 );

	if (pthread_create(&threads[0], NULL, &thread_wiegand_out, NULL) == -1)
	{
		//writeLogV2(LOGPATH, NAME, "thread_led create error\n");
	}
	if (pthread_create(&threads[1], NULL, &redis_GPIO_REV, NULL) == -1)
	{
		//writeLogV2(LOGPATH, NAME, "thread_led create error\n");
	}

	unsigned char bitCnt = 0;
	unsigned char preBitCnt = 0;
	unsigned int csn = 0;
	int sensing = 0;

	int rtn = 0;
	char testData[64];

	while(running){


		/*
		   memset(testData, 0, 64);
		   rtn = read(fd,testData, 64);
		   if( rtn > 0 )
		   {
		   printf("detect data(%d byte) : ", rtn);
		   for( i = 0; i < rtn; i++ )
		   printf("%02X(%d) | ", testData[i], testData[i]);
		   printf("\n");
		   }



		   if( gbWgInCur >= 34 )
		//if( gbWgInCur >= 35 )
		{

		// 34bit
		if( gConfig.cardType == 1 ) {
		Card_34Bit_Format();
		}
		// 35bit
		else if( gConfig.cardType == 2 ) {
		Card_35Bit_Format();
		}

		//oddParityCnt = 0;
		//evenParityCnt = 0;
		//card = 0;
		gbWgInCur = 0;
		}

		//delay(100);	
		 */
		// digitalWrite(D0, 	HIGH);
		// digitalWrite(D1, 	HIGH);
		// usleep(1000 * 1000);	// 100ms

		// digitalWrite(D0, 	LOW);
		// digitalWrite(D1, 	LOW);
		usleep(1000 * 1000);	// 100ms

	}
	/*******************************************************/

	//writeLogV2(LOGPATH, NAME, "main() End\n");
	return 0;


}

void Card_26Bit_Format()
{
	int i;

	unsigned int card = 0;
	unsigned int facilityCode= 0;

	printf("Standard 26-Bit Wiegand Format\n");
	printf("[1] Leading Parity Bit(Even) : %d\n", wgbuf[0] );

	printf("[2~9] Facility Code : ");
	for( i = 1; i < 9; i++ ) {
		facilityCode |= wgbuf[i] << (8-i);
		printf("%d", wgbuf[i] );
	}
	printf(" ( %d )", facilityCode);
	printf("\n");
	printf("[10~25] Card Code : ");
	for( i = 9; i < 25; i++ ) {
		card |= wgbuf[i] << (24-i);
		printf("%d", wgbuf[i] );
	}

	printf(" ( %d )", card);
	printf("\n");
	printf("[26] Trailing Parity Bit(Odd) : %d\n\n", wgbuf[25] );


	redisCommand(event,"PUBLISH Verify 0;4;%ld;%d;%d;", card, gConfig.cardType, facilityCode);	// changed to support multi-card format 170904

	// by pass test
	printf("by pass (wiegand out)\n");
	//for( i = 33; i >= 0; i-- ) {
	for( i = 0; i < 26; i++ ) {

		if( wgbuf[i] == 0 ) outD0();
		else outD1();
		//usleep(1000);	// 1ms
		usleep(gConfig.interPulseGap);	// add 170904 for ini parser
	}
	printf("\n");
	printf("\n");
	printf("\n");


}
void Card_34Bit_Format()
{
	int i = 0;
	int bitCount = 34;
	//long card = 0;
	unsigned int card = 0;

	char oddParity = 0;
	char evenParity = 0;

	int oddParityCnt = 0;
	int evenParityCnt = 0;


	printf("Wiegand In : \n");
	for( i = 0; i < bitCount; i++ )
	{
		printf("%d", wgbuf[i]);
		//if( i == 0 ) oddParity = wgbuf[i];
		//if( i == 33 ) evenParity = wgbuf[i];
	}
	printf("\n");

	//printf("odd %d, even %d\n", oddParity, evenParity);

	for( i = 1; i < bitCount-1; i++ )
	{
		card |= wgbuf[i] << (32-i);

		if( i < 17 ) {
			if( wgbuf[i] == 1 ) oddParityCnt++;
		}
		else {
			if( wgbuf[i] == 1 ) evenParityCnt++;
		}

	}
	printf(" CSN : %ld\n", (long)card);
	//redisCommand(event,"PUBLISH Verify 0;4;%ld;", card);

	redisCommand(event,"PUBLISH Verify 0;4;%ld;%d;%d;", card, gConfig.cardType, 0  );	// changed to support multi-card format 170904

	if( oddParityCnt % 2 ) {
		if( oddParity != 1 ) printf("oddParity error : %d\n", oddParity);
		else printf("oddParity success : %d\n", oddParity);
	}
	else {
		if( oddParity != 0 ) printf("oddParity error : %d\n", oddParity);
		else printf("oddParity success : %d\n", oddParity);
	}

	if( evenParityCnt % 2 ) {
		if( evenParity != 0 ) printf("evenParity error : %d\n", evenParity);
		else printf("evenParity success : %d\n", evenParity);
	}
	else {
		if( evenParity != 1 ) printf("evenParity error : %d\n", evenParity);
		else printf("evenParity success : %d\n", evenParity);
	}

	// by pass test
	printf("by pass (34 Bit : wiegand out)\n");
	//for( i = 33; i >= 0; i-- ) {
	for( i = 0; i < 34; i++ ) {

		if( wgbuf[i] == 0 ) outD0();
		else outD1();
		//usleep(1000);	// 1ms
		usleep(gConfig.interPulseGap);	// add 170904 for ini parser
	}
	printf("\n");
	printf("\n");
	printf("\n");



}
void Card_35Bit_Format()
{
	int i;

	unsigned int card = 0;
	unsigned int facilityCode= 0;

	printf("[1] Odd Parity Bits(All 35 bits) : %d\n", wgbuf[0] );
	printf("[2] Even Parity Bits : %d\n", wgbuf[1] );

	printf("[3~14] Facility Code : ");
	for( i = 2; i < 14; i++ ) {
		facilityCode |= wgbuf[i] << (13-i);
		printf("%d", wgbuf[i] );
	}
	printf(" ( %d )", facilityCode);
	printf("\n");
	printf("[15~34] Card Code : ");
	for( i = 14; i < 34; i++ ) {
		card |= wgbuf[i] << (33-i);
		printf("%d", wgbuf[i] );
	}

	printf(" ( %d )", card);
	printf("\n");
	printf("[35] Odd Parity Bits : %d\n\n", wgbuf[34] );


	redisCommand(event,"PUBLISH Verify 0;4;%ld;%d;%d;", card, gConfig.cardType, facilityCode);	// changed to support multi-card format 170904

	// by pass test
	printf("by pass (wiegand out)\n");
	//for( i = 33; i >= 0; i-- ) {
	for( i = 0; i < 35; i++ ) {

		if( wgbuf[i] == 0 ) outD0();
		else outD1();
		//usleep(1000);	// 1ms
		usleep(gConfig.interPulseGap);	// add 170904 for ini parser
	}
	printf("\n");
	printf("\n");
	printf("\n");


}



void *thread_wiegand_out(void *arg)
{

	//writeLogV2(LOGPATH, NAME, "start wiegand_out\n");
	//debugPrintf(configInfo.debug, "start wiegand_out");

	struct event_base *base = event_base_new();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err)
	{
		// Let *c leak for now...
		//debugPrintf(configInfo.debug, "Error: %s\n", c->errstr);
		return 0;
	}

	redisLibeventAttach(c, base);

	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE BACSCore_Wiegand");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE GPIO_REV");
	// redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE Verify");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE Wiegand_In");

	event_base_dispatch(base);
	exit(1);

	//writeLogV2(LOGPATH, NAME, "End wiegand_out\n");
	//debugPrintf(configInfo.debug, "End wiegand_out");

	pthread_exit((void *) 0);
}

void *redis_GPIO_REV(void *arg)
{

	//writeLogV2(LOGPATH, NAME, "start wiegand_out\n");
	//debugPrintf(configInfo.debug, "start wiegand_out");

	struct event_base *base = event_base_new();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err)
	{
		// Let *c leak for now...
		//debugPrintf(configInfo.debug, "Error: %s\n", c->errstr);
		return 0;
	}

	redisLibeventAttach(c, base);

	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE GPIO_REV");

	event_base_dispatch(base);
	exit(1);

	//writeLogV2(LOGPATH, NAME, "End wiegand_out\n");
	//debugPrintf(configInfo.debug, "End wiegand_out");

	pthread_exit((void *) 0);
}

void sig_handler(int signo)
{
	int i;
	int rc;
	int status;

	running = 0;

	printf("ctrl-c\n");

	for (i = 4; i >= 0; i--)
	{
		rc = pthread_cancel(threads[i]); // 강제종료
		if (rc == 0)
		{
			// 자동종료
			rc = pthread_join(threads[i], (void **) &status);
			if (rc == 0)
			{
				printf("Completed join with thread %d status= %d\n", i, status);
			}
			else
			{
				printf(
						"ERROR; return code from pthread_join() is %d, thread %d\n",
						rc, i);
			}
		}
	}
	exit(1);
}


int bitCount(long long int_type)
{
	int count = 0;
	while(int_type) {
		int_type &= int_type -1;
		count += 1;
	}
	return count;
}

long long generate26bitHex(long long facilityCode, long long cardCode)
{
	long long cardData = (facilityCode << 17) + (cardCode << 1);
	// MSB even parity (covers 12 MSB)
	long long parity1 = bitCount(cardData & 0x1FFE000) & 1;
	// LSB odd parity (covers 12 LSB)
	long long parity2 = bitCount(cardData & 0x0001FFE) & 1 ^ 1;
	cardData += (parity1 << 25) + (parity2);
	return cardData;
}
long long generate35bitHex(long long facilityCode, long long cardCode)
{
	long long cardData = (facilityCode << 21) + (cardCode << 1);
	long long parity1 = bitCount(cardData & 0x1B6DB6DB6) & 1;
	cardData += (parity1 << 33);
	long long parity2 = bitCount(cardData & 0x36DB6DB6C) & 1 ^ 1;
	cardData += parity2;
	long long parity3 = bitCount(cardData) & 1 ^ 1;
	cardData += (parity3 << 34);
	return cardData;
}

static void onMessage(redisAsyncContext *c, void *reply, void *privdata)
{

	int status, offset;
	int i, j, rtrn, len;
	redisReply *r = reply; 
	int cardType = 0;
	long long card;
	long long result;
	long long fdata = 1630;
	//long long int card;
	//unsigned int card2;
	char wgOutBuf[64];
	char testbuff[64];
	int oddParityCnt = 0;
	int evenParityCnt = 0;


	int cnttest = 0;

	char str[1024];
	char *ptr;
	if (reply == NULL)
		return;

	if (r->type == REDIS_REPLY_ARRAY)
	{
//		for (j = 0; j < r->elements; j++)
//		{
//			printf("%u) %s\n", j, r->element[j]->str);
//		}

		if (strcasecmp(r->element[0]->str, "message") == 0	&& strcasecmp(r->element[1]->str, "GPIO_REV") == 0)
		{
			len = strlen(r->element[2]->str);

			if (len <= 0)
				return;

			strcpy(str, r->element[2]->str);

			ptr = strtok(str, ";");
			if (ptr == NULL)
				return;
			strcpy(gpio_key, ptr);

			ptr = strtok( NULL, ";");
			if (ptr == NULL)
				return;
			strcpy(gpio_value, ptr);

			printf("key = %s, value = %s \n\r", gpio_key, gpio_value);

			//WIEGAND D0
			if		((strcasecmp(gpio_key, "11") == 0) && (strcasecmp(gpio_value, "true") == 0))
			{
				digitalWrite(D0, 	HIGH);

				printf("GPIO_REV : Wiegand D0 HIGH \n\r");
			}
			else if	((strcasecmp(gpio_key, "11") == 0) && (strcasecmp(gpio_value, "false") == 0))
			{
				digitalWrite(D0,	LOW);
				printf("GPIO_REV : Wiegand D0 LOW \n\r");
			}
			//WIEGAND D1
			if		((strcasecmp(gpio_key, "6") == 0) && (strcasecmp(gpio_value, "true") == 0))
			{
				digitalWrite(D1, 	HIGH);

				printf("GPIO_REV : Wiegand D1 HIGH \n\r");
			}
			else if	((strcasecmp(gpio_key, "6") == 0) && (strcasecmp(gpio_value, "false") == 0))
			{
				digitalWrite(D1,	LOW);
				printf("GPIO_REV : Wiegand D1 LOW \n\r");
			}
		}

		else if (strcasecmp(r->element[0]->str, "message") == 0
				&& strcasecmp(r->element[1]->str, "BACSCore_Wiegand") == 0 )
		{
			// set wiegand output 
			len = strlen(r->element[2]->str);

			if( len <= 0 ) return;



			//redisCommand(event,"PUBLISH Verify 0;4;%d;%d;%ld;", gConfig.cardType, facilityCode, card);	// changed to support multi-card format 170904



			strcpy(str, r->element[2]->str);
			ptr = strtok( str, ";");
			if( ptr != NULL ){
				card = atoll(ptr);
				printf("atoll(lld) = %lld\n", card);
			}

			ptr = strtok( NULL, ";");
			if( ptr != NULL ){
				cardType = atoi(ptr);
				printf("atoi(d) = %d\n", cardType);
			}

			ptr = strtok( NULL, ";");
			if( ptr != NULL ){
				fdata = atoll(ptr);
				printf("atoll(lld) = %lld\n", fdata);
			}

			// ADD -> 171103, facilityCode (web setting), Justin
			cardType = gConfig.cardType;
			fdata = gConfig.facilityCode;
			printf("CardType = %d, FacilityCode =  %lld\n", cardType, fdata);

			// 26 Bit format
			if( cardType == 0 ) {

				result = generate26bitHex(fdata, card);

				for( i = 0; i < 26; i++ )
				{
					wgOutBuf[i] = (char)(0x01 & (result >> 25-i));
				}

				printf("Standard 26-Bit Wiegand Format\n");
				printf("[1] Leading Parity Bit(Even) : %d\n", wgOutBuf[0] );

				printf("[2~9] Facility Code : ");
				for( i = 1; i < 9; i++ ) {
					printf("%d", wgOutBuf[i] );
				}
				printf("\n");
				printf("[10~25] Card Code : ");
				for( i = 9; i < 25; i++ ) {
					printf("%d", wgOutBuf[i] );
				}
				printf("\n");
				printf("[26] Trailing Parity Bit(Odd) : %d\n\n", wgOutBuf[25] );


				printf("Wiegand Out\n");
				for( i = 0; i < 26; i++ ) {
					//for( i = 34; i >= 0; i-- ) {

					if( wgOutBuf[i] == 0 ) outD0();
					else outD1();
					usleep(gConfig.interPulseGap);	// add 170904 for ini parser
					//usleep(1000);	// 1ms
				}
				printf("\n");
				printf("\n");
				printf("\n");


			}
			// 34 Bit format
			else if( cardType == 1 ) {


				printf("(char)(0x01 & (card >> i-1)) : ");
				for( i = 1; i < 34-1; i++ )
				{
					//wgOutBuf[i] = (char)(0x01 & (card >> 33-i));
					wgOutBuf[i] = (char)(0x01 & (card >> i-1));
					printf("%d[%d][%d] ", 
							wgOutBuf[i],
							evenParityCnt,
							oddParityCnt
						  );
					//wgOutBuf[i] = (char)(0x01 & (card2 >> i-1));

					if( i < 17 ){
						if( wgOutBuf[i] == 1 ) evenParityCnt++;
					}
					else
						if( wgOutBuf[i] == 1 ) oddParityCnt++;

				}
				printf("\n");

				//unsigned int cardtest = 0;
				long long cardtest = 0;
				for( i = 1; i < 34-1; i++ ) {
					cardtest |= wgOutBuf[i] << (i-1);
				}
				printf("re parser value = %lld\n", cardtest);
//				printf("evenParityCnt(evenParityCnt % 2) = %d %d \n", evenParityCnt, evenParityCnt % 2);
//				printf("oddParityCnt(oddParityCnt % 2) = %d %d \n", oddParityCnt, oddParityCnt % 2);


				if( oddParityCnt % 2 ) wgOutBuf[33] = 1;
				else wgOutBuf[33] = 0;

				if( evenParityCnt % 2 ) wgOutBuf[0] = 0;
				else wgOutBuf[0] = 1;

				printf("wgOutBuf[0] = %d\n", wgOutBuf[0]);
				printf("wgOutBuf[33] = %d\n", wgOutBuf[33]);

				printf("RFID : ");
				//for( i = 0; i < 34; i++ ) {
				for( i = 33; i >= 0; i-- ) {

					if( wgOutBuf[i] == 0 ) outD0();
					else outD1();
					//usleep(1000);	// 1ms
					usleep(gConfig.interPulseGap);	// add 170904 for ini parser
				}
				printf("\n");
				printf("system -> write\n");


			}
			// 35 Bit format
			else if( cardType == 2 ) {

				result = generate35bitHex(fdata, card);

				for( i = 0; i < 35; i++ )
				{
					wgOutBuf[i] = (char)(0x01 & (result >> 34-i));
				}

				//This format uses a 12 bit facility code (bits 3-14) and a 20 bit card code (bits 15-34). Bit 1 is an odd parity bit that covers all 35 bits. Bit 2 is an even parity covering bits 3,4,6,7,9,10,12,13,15,16,28,19,21,22,24,25,27,28,30,31,33,34. Bit 35 is odd parity, covering bits 2,3,5,6,8,9,11,12,14,15,17,18,20,21,23,24,26,27,29,30,32,33. When calculating the parity bits, you must calculate bit 2, bit 35, and finally bit 1. What a weird format. I'm guessing this parity scheme is supposed to add another layer of obscurity.
				printf("[1] Odd Parity Bits(All 35 bits) : %d\n", wgOutBuf[0] );
				printf("[2] Even Parity Bits : %d\n", wgOutBuf[1] );

				printf("[3~14] Facility Code : ");
				for( i = 2; i < 14; i++ ) {
					printf("%d", wgOutBuf[i] );
				}
				printf("\n");
				printf("[15~34] Card Code : ");
				for( i = 14; i < 34; i++ ) {
					printf("%d", wgOutBuf[i] );
				}
				printf("\n");
				printf("[35] Odd Parity Bits : %d\n\n", wgOutBuf[34] );


				printf("Wiegand Out\n");
				for( i = 0; i < 35; i++ ) {
					//for( i = 34; i >= 0; i-- ) {

					if( wgOutBuf[i] == 0 ) outD0();
					else outD1();
					usleep(gConfig.interPulseGap);	// add 170904 for ini parser
					//usleep(1000);	// 1ms
				}
				printf("\n");
				printf("\n");
				printf("\n");
			}

		}
		else if (strcasecmp(r->element[0]->str, "message") == 0
				&& strcasecmp(r->element[1]->str, "Verify") == 0 )	//use isc west
		{
			strcpy(str, r->element[2]->str);
			ptr = strtok(str, ";");
			printf("redis verify data : %s \n\r", ptr);

			if ((strcasecmp(ptr, "photo") == 0))
			{
				// printf("photo");
				// digitalWrite(D0, 	LOW);
				// usleep(1000*300);
				// digitalWrite(D0, 	HIGH);
				// usleep(1000*300);
			}
			else if ((strcasecmp(ptr, "fail") == 0))
			{
				// printf("fail");
				// digitalWrite(D1, 	LOW);
				// usleep(1000*300);
				// digitalWrite(D1, 	HIGH);
				// digitalWrite(D1,	HIGH);
			}
			else
			{

			}
		}
		else if (strcasecmp(r->element[0]->str, "message") == 0
				&& strcasecmp(r->element[1]->str, "Wiegand_In") == 0 )
		{
			// set wiegand output
			len = strlen(r->element[2]->str);

			if( len <= 0 ) return;
			//redisCommand(event,"PUBLISH Verify 0;4;%d;%d;%ld;", gConfig.cardType, facilityCode, card);	// changed to support multi-card format 170904
			strcpy(str, r->element[2]->str);
			printf("Wiegand_In message : %s \n\r", str);
			ptr = strtok( str, ";");
			if( ptr != NULL )
			{
				if (strcasecmp(ptr, "err") == 0)
				{

				}
				else
				{
					ptr = strtok( NULL, ";");
					if( ptr != NULL )
					{
						// sprintf(cardType, "%d", atoi(ptr));
						cardType = atoi(ptr);
						// printf("debug type : %s \n\r", ptr);
						ptr = strtok( NULL, ";");
						if( ptr != NULL )
						{
							// sprintf(card, "%ll", atoll(ptr));
							printf("debug card : %s \n\r", ptr);
							// card = atoll(ptr);
							card = strtoll(ptr, NULL, 16);
							char hexStringBuf[10];
							strcpy(hexStringBuf, ptr);


							printf("type : %d bit, card : %lld \n\r", cardType, card );
							char pubMessage[100] = "";
							memset(pubMessage, 0, sizeof(pubMessage));
							sprintf(pubMessage, "PUBLISH Synergis {\"status\":\"REPORT_CARD_SWIPE\",\"bitcount\":\"%d\",\"cardcode\":\"%d\"}", cardType, (int)card);
							redisCommand(event, pubMessage);
							printf("publish message : %s \n\r", pubMessage);

							// 26 Bit format
							if( cardType == 24 ) {
								printf("card type : %d  \n\r", cardType);
								char fDataString[2];
								char cDataString[4];
								sprintf(fDataString, "%c%c", 		hexStringBuf[0], hexStringBuf[1]);
								sprintf(cDataString, "%c%c%c%c", 	hexStringBuf[2], hexStringBuf[3], hexStringBuf[4], hexStringBuf[5]);
								long long recieveFdata, recieveCdata;
								recieveFdata = strtoll(fDataString, NULL, 16);
								recieveCdata = strtoll(cDataString, NULL, 16);

								printf("debug1 : %lld \n\r", recieveFdata);
								printf("debug2 : %lld \n\r", recieveCdata);																

								result = generate26bitHex(recieveFdata, recieveCdata);

								for( i = 0; i < 26; i++ )
								{
									wgOutBuf[i] = (char)(0x01 & (result >> 25-i));
								}

								printf("Standard 26-Bit Wiegand Format\n");
								printf("[1] Leading Parity Bit(Even) : %d\n", wgOutBuf[0] );

								printf("[2~9] Facility Code : ");
								for( i = 1; i < 9; i++ ) {
									printf("%d", wgOutBuf[i] );
								}
								printf("\n");
								printf("[10~25] Card Code : ");
								for( i = 9; i < 25; i++ ) {
									printf("%d", wgOutBuf[i] );
								}
								printf("\n");
								printf("[26] Trailing Parity Bit(Odd) : %d\n\n", wgbuf[25] );


								printf("Wiegand Out\n");
								for( i = 0; i < 26; i++ ) {
									//for( i = 34; i >= 0; i-- ) {

									if( wgOutBuf[i] == 0 ) outD0();
									else outD1();
									usleep(gConfig.interPulseGap);	// add 170904 for ini parser
									//usleep(1000);	// 1ms
								}
								printf("\n");
								printf("\n");
								printf("\n");


							}
							// 34 Bit format
							else if( cardType == 32 ) {

								printf("card type : %d  \n\r", cardType);
								printf("(char)(0x01 & (card >> i-1)) : ");
								for( i = 1; i < 34-1; i++ )
								{
									//wgOutBuf[i] = (char)(0x01 & (card >> 33-i));
									wgOutBuf[i] = (char)(0x01 & (card >> i-1));
									printf("%d[%d][%d] ",
											wgOutBuf[i],
											evenParityCnt,
											oddParityCnt
										);
									//wgOutBuf[i] = (char)(0x01 & (card2 >> i-1));

									if( i < 17 ){
										if( wgOutBuf[i] == 1 ) evenParityCnt++;
									}
									else
										if( wgOutBuf[i] == 1 ) oddParityCnt++;

								}
								printf("\n");

								//unsigned int cardtest = 0;
								long long cardtest = 0;
								for( i = 1; i < 34-1; i++ ) {
									cardtest |= wgOutBuf[i] << (i-1);
								}
								printf("re parser value = %lld\n", cardtest);
								// printf("evenParityCnt(evenParityCnt % 2) = %d %d\n", evenParityCnt, (evenParityCnt % 2));
								// printf("oddParityCnt(oddParityCnt % 2) = %d %d\n", oddParityCnt, (oddParityCnt % 2));


								if( oddParityCnt % 2 ) wgOutBuf[33] = 1;
								else wgOutBuf[33] = 0;

								if( evenParityCnt % 2 ) wgOutBuf[0] = 0;
								else wgOutBuf[0] = 1;

								printf("wgOutBuf[0] = %d\n", wgOutBuf[0]);
								printf("wgOutBuf[33] = %d\n", wgOutBuf[33]);

								printf("RFID : ");
								//for( i = 0; i < 34; i++ ) {
								for( i = 33; i >= 0; i-- ) {

									if( wgOutBuf[i] == 0 ) outD0();
									else outD1();
									//usleep(1000);	// 1ms
									usleep(gConfig.interPulseGap);	// add 170904 for ini parser
								}
								printf("\n");
								printf("system -> write\n");


							}
							else
							{
								printf("undefined card type : %d  \n\r", cardType);
								printf("(char)(0x01 & (card >> i-1)) : ");
								for( i = 1; i < 34-1; i++ )
								{
									//wgOutBuf[i] = (char)(0x01 & (card >> 33-i));
									wgOutBuf[i] = (char)(0x01 & (card >> i-1));
									printf("%d[%d][%d] ",
											wgOutBuf[i],
											evenParityCnt,
											oddParityCnt
										);
									//wgOutBuf[i] = (char)(0x01 & (card2 >> i-1));

									if( i < 17 ){
										if( wgOutBuf[i] == 1 ) evenParityCnt++;
									}
									else
										if( wgOutBuf[i] == 1 ) oddParityCnt++;

								}
								printf("\n");

								//unsigned int cardtest = 0;
								long long cardtest = 0;
								for( i = 1; i < 34-1; i++ ) {
									cardtest |= wgOutBuf[i] << (i-1);
								}
								printf("re parser value = %lld\n", cardtest);
								// printf("evenParityCnt(evenParityCnt % 2) = %d %d\n", evenParityCnt, (evenParityCnt % 2));
								// printf("oddParityCnt(oddParityCnt % 2) = %d %d\n", oddParityCnt, (oddParityCnt % 2));


								if( oddParityCnt % 2 ) wgOutBuf[33] = 1;
								else wgOutBuf[33] = 0;

								if( evenParityCnt % 2 ) wgOutBuf[0] = 0;
								else wgOutBuf[0] = 1;

								printf("wgOutBuf[0] = %d\n", wgOutBuf[0]);
								printf("wgOutBuf[33] = %d\n", wgOutBuf[33]);

								printf("RFID : ");
								//for( i = 0; i < 34; i++ ) {
								for( i = 33; i >= 0; i-- ) {

									if( wgOutBuf[i] == 0 ) outD0();
									else outD1();
									//usleep(1000);	// 1ms
									usleep(gConfig.interPulseGap);	// add 170904 for ini parser
								}
								printf("\n");
								printf("system -> write\n");

							}
							

						}							
					}										
				}
			} 
		}		
		else
		{
			//not todo
		}

	}
}

		
