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
#include "gpio_control.h"
#include <stdlib.h> 
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <pthread.h>
#include "./lib/parser/ini.h"
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>

#include <wiringPi.h>


//wiringPi port
#define NETWORK_RESET	23

void wiringPi_io_setup(void)
{
	int ret = wiringPiSetup();
	if(ret == 0)	printf("wiringPi init success \n\r");

	pinMode(NETWORK_RESET, 			INPUT);

}


#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

typedef struct
{
	const char* useRelay0;
	const char* useRelay1;
	int grantAccessTime0;
	int grantAccessTime1;
	int modelType;
} Configuration;

static int handler(void* config, const char* section, const char* name, const char* value)
{
	Configuration* pconfig = (Configuration*)config;

	if (MATCH("relay", "useRelay0")) {
		pconfig->useRelay0 = strdup(value);
	} else if (MATCH("relay", "useRelay1")) {
		pconfig->useRelay1 = strdup(value);
	} else if (MATCH("relay", "grantAccessTime0")) {
		pconfig->grantAccessTime0 = atoi(value);
	} else if (MATCH("relay", "grantAccessTime1")) {
		pconfig->grantAccessTime1 = atoi(value);
	}
	else if (MATCH("uart", "modelType")) {
			pconfig->modelType = atoi(value);
	}
	else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}


Configuration gConfig;

pthread_t threads[2];
redisContext *event;


void *redis_Verify(void *arg);
//void *duogpi_thread_function(void *arg);
void *test_thread_function(void *arg);
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);


redisContext* redisInitialize() {
    
    redisContext* c;
    const char *hostname = "127.0.0.1";
    //const char *hostname = "192.168.100.110";
    int port = 6379;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds

    c = redisConnectWithTimeout(hostname, port, timeout);

    if (c == NULL || c->err) {
	}


    return c;

}

void *redis_Verify(void *arg)
{

	//writeLogV2(LOGPATH, NAME, "start wiegand_out\n");
	//debugPrintf(configInfo.debug, "start wiegand_out");

	struct event_base *base = event_base_new();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err)
	{
		// Let *c leak for now...
		//debugPrintf(configInfo.debug, "Error: %s\n", c->errstr);
		//return;
		pthread_exit((void *) 0);
	}

	redisLibeventAttach(c, base);

	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE BACSCore_GateOpen");

	event_base_dispatch(base);
	exit(1);

	//writeLogV2(LOGPATH, NAME, "End wiegand_out\n");
	//debugPrintf(configInfo.debug, "End wiegand_out");

	pthread_exit((void *) 0);
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
		for (j = 0; j < r->elements; j++)
		{
			printf("%u) %s\n", j, r->element[j]->str);

		}
		if (strcasecmp(r->element[0]->str, "message") == 0
				&& (strcasecmp(r->element[1]->str, "BACSCore_GateOpen") == 0))
		{

			if (strcasecmp(r->element[2]->str, "out;0;low;") == 0 ) {

//				gpio_set_value(10, 0);	// port, Low
				printf("LOW\n");
			}
			else if (strcasecmp(r->element[2]->str, "out;0;high;") == 0 ) {

//				gpio_set_value(10, 1);	// port, High
				printf("HIGH\n");

			}
			else if (strcasecmp(r->element[2]->str, "1") == 0 )
			{
				// door open
//				gpio_set_value(10, 1);	// high
//				usleep(gConfig.grantAccessTime0*1000);
//				gpio_set_value(10, 0);	// low

			}
			else
			{
				printf("Wrong message.");
			}
		}

	}
}



int main(int argc, char **argv)
{

	/* without wiringPi
	 *
	//init gpio
	gpio_open_port(10);		//pin open
	gpio_open_port(11);		//pin open
	gpio_init_port(10, 0);	//pin output set
	gpio_init_port(11, 0);	//pin output set

	*/
	wiringPi_io_setup();

	if (pthread_create(&threads[0], NULL, &redis_Verify, NULL) == -1)
	{
		//writeLogV2(LOGPATH, NAME, "thread_led create error\n");
	}

	event = redisInitialize();

	memset( &gConfig, 0, sizeof( Configuration ) );	// add 170905 for ini parser
	if (ini_parse("settings.ini", handler, &gConfig) < 0) {
		printf("Can't load 'settings.ini'\n");
		return 1;
	}
	printf("Config loaded from 'settings.ini': useRelay0=%s,  useRelay1=%s, grantAccessTime0=%d, grantAccessTime1=%d\n",
			gConfig.useRelay0, gConfig.useRelay1, gConfig.grantAccessTime0, gConfig.grantAccessTime1);
	printf("Config load from 'settings.ini': modelType : %d \n\r", 	gConfig.modelType);

	int ret;
	int btnCount = 0;

    while(1)
    {
    	usleep(1000 * 100); //100ms process

    	printf("pin : %d, Value : %d \n\r", NETWORK_RESET, digitalRead(NETWORK_RESET));

    	if(digitalRead(NETWORK_RESET)) 	//push : active High
    	{
			btnCount ++;
			if( (btnCount > 0) && ((btnCount % 10) == 0) )
			{
				char publishCMD[100];
				memset(publishCMD, 0, sizeof(publishCMD));
				sprintf(publishCMD, "count : %d", btnCount);
				redisCommand(event, publishCMD);
				printf("redis pub : %s \n\r", publishCMD);				
			}
    	}
    	else							// release : default low
    	{

			if(btnCount > 10 && btnCount < 50)
			{
				printf("H/W Reset! \n\r");
				char publishCMD[] =	{ "PUBLISH Verify guide;reboot...;" };
				redisCommand(event, publishCMD);
				printf("redis pub : %s \n\r", publishCMD);

				char publishCMD1[] ={ "PUBLISH BACSDuoUart RESET" };
				redisCommand(event, publishCMD1);
				printf("redis pub : %s \n\r", publishCMD1);
			}
    		else if(btnCount >= 50 && btnCount < 100)
    		{
    			printf("network reset! \n\r");
				char publishCMD[] =	{ "PUBLISH Verify guide;network_reset;" };
				redisCommand(event, publishCMD);
				printf("redis pub : %s \n\r", publishCMD);
    			ret = system("sudo cp -r /home/bacs/factorydefault/interfaces /etc/network/");
    			ret = system("sudo cp -r /home/bacs/factorydefault/network.json /home/bacs/host/ini/network.json");

				usleep(1000*1000);
				ret = system("sudo sync");
				ret = system("reboot");
    		}
    		else if (btnCount > 100)
    		{
    			printf("factory default! \n\r");
				char publishCMD[] =	{ "PUBLISH Verify guide;factory_default;" };
				redisCommand(event, publishCMD);
				printf("redis pub : %s \n\r", publishCMD);

    			ret = system("/home/bacs/factorydefault/factorydefault.sh");
				printf("factory default scrpit result : %d \n\r", ret);

				usleep(1000*1000);
				ret = system("sudo sync");
				ret = system("reboot");
    		}

    		btnCount = 0;
    	}
    	printf("test count : %d \n\r", btnCount);
    }
}


