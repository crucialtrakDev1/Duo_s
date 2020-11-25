/*
 ============================================================================
 Name        : main.c
 Author      : winston
 Version     :
 Copyright   : Your copyright notice
 ============================================================================
 */

#define bacs_duo_main_src

//#define TFTLCD
//#define GPIO
//#define I2C
//#define hiredis
//#define RING_I2C
//#define LED_TEST_CODE

//-----------------------------------------------------------------------------------------------------------------------------------

#ifdef bacs_duo_main_src

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

#include "tofM.h"
#include "./lib/parser/ini.h"
#include "tft_lcd_control.h"
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>
#include <wiringPi.h>

#define XSHUT	21

void wiringPi_io_setup(void)
{
	int ret = wiringPiSetup();
	if(ret == 0)	printf("wiringPi init success \n\r");

	pinMode			(XSHUT, OUTPUT);
	digitalWrite 	(XSHUT, HIGH);
}


#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

typedef struct {
	int tofmin;
	int tofmax;
	int toofar;
	int tooclose;
} Configuration;

static int handler(void* config, const char* section, const char* name,
		const char* value) {
	Configuration* pconfig = (Configuration*) config;

	if (MATCH("system", "tofmin")) {
		pconfig->tofmin = atoi(value);
	} else if (MATCH("system", "tofmax")) {
		pconfig->tofmax = atoi(value);
	} else if (MATCH("system", "toofar")) {
		pconfig->toofar = atoi(value);
	} else if (MATCH("system", "tooclose")) {
		pconfig->tooclose = atoi(value);
	} else {
		return 0; /* unknown section/name, error */
	}
	return 1;
}

int g_in_out = 0;
int gFlagStart = 0;
	int exit_tmr_count = 0;


Configuration gConfig;

pthread_t threads[4];
bio_state bio;
redisContext *event;

int strUpr(char *str) {

	int loop = 0;
	while (str[loop] != '\0') {
		str[loop] = (char) toupper(str[loop]);
		loop++;
	}
	return loop;
}

void *redis_Verify(void *arg);
void *thread_verify(void *arg);
void *tmr_exit(void *arg);
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);
void sned_Tof_Value(int value, int in_out);

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

void *redis_Verify(void *arg) {

	//writeLogV2(LOGPATH, NAME, "start wiegand_out\n");
	//debugPrintf(configInfo.debug, "start wiegand_out");

	struct event_base *base = event_base_new();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err) {
		// Let *c leak for now...
		//debugPrintf(configInfo.debug, "Error: %s\n", c->errstr);
		//return;
		pthread_exit((void *) 0);
	}

	redisLibeventAttach(c, base);

	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE bacstof");

	event_base_dispatch(base);
	exit(1);

	//writeLogV2(LOGPATH, NAME, "End wiegand_out\n");
	//debugPrintf(configInfo.debug, "End wiegand_out");

	pthread_exit((void *) 0);
}

void *thread_verify(void *arg) {

	//writeLogV2(LOGPATH, NAME, "start wiegand_out\n");
	//debugPrintf(configInfo.debug, "start wiegand_out");

	struct event_base *base = event_base_new();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err) {
		// Let *c leak for now...
		//debugPrintf(configInfo.debug, "Error: %s\n", c->errstr);
		//return;
		pthread_exit((void *) 0);
	}

	redisLibeventAttach(c, base);

	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE Verify");

	event_base_dispatch(base);
	exit(1);

	//writeLogV2(LOGPATH, NAME, "End wiegand_out\n");
	//debugPrintf(configInfo.debug, "End wiegand_out");

	pthread_exit((void *) 0);
}


void *tmr_exit(void *arg) 
{

	while(1)
	{
		printf("exit tmr count = %d \n\r", exit_tmr_count);
		usleep(1000 * 100);
		if(exit_tmr_count > 150)
		{
			// exit(1);
		}
		else
		{
			exit_tmr_count++;
			/* code */
		}
		

	}
}



static void onMessage(redisAsyncContext *c, void *reply, void *privdata) {

	int i, j, rtrn, len;
	redisReply *r = reply;

	char str[1024];
	char *ptr;

	int min = 0;
	int max = 0;
	if (reply == NULL)
		return;

	if (r->type == REDIS_REPLY_ARRAY) {
		for (j = 0; j < r->elements; j++) {
//			printf("%u) %s\n", j, r->element[j]->str);

		}
		if (strcasecmp(r->element[0]->str, "message") == 0
				&& strcasecmp(r->element[1]->str, "bacstof") == 0) {

			// publish bacstof setup;0;700;
			len = strlen(r->element[2]->str);

			if (len <= 0)
				return;

			strcpy(str, r->element[2]->str);
			ptr = strtok(str, ";");
			if (ptr == NULL)
				return;

			if (strcmp("setup", ptr) == 0) {

				printf("setup\n");

				// min
				ptr = strtok( NULL, ";");
				if (ptr == NULL)
					return;
				min = atoi(ptr);
				if (min < 0 || 1500 < min)
					min = 0;
				gConfig.tofmin = min;

				// max
				ptr = strtok( NULL, ";");
				if (ptr == NULL)
					return;
				max = atoi(ptr);
				if (max < 0 || 1500 < max)
					max = 700;
				gConfig.tofmax = max;

				printf("new accept config : min = %d, max = %d \n\r",
						gConfig.tofmin, gConfig.tofmax);

			}
		}
	}
}

int main(int argc, char **argv) {
	//tof values
	VL53L0X_Dev_t MyDevice;
	VL53L0X_Dev_t *pMyDevice = &MyDevice;
	uint16_t tof_value;

	wiringPi_io_setup();

	int tof_open_return = TOF_OPEN_FAIL;

	if (pthread_create(&threads[0], NULL, &redis_Verify, NULL) == -1) {
		//writeLogV2(LOGPATH, NAME, "thread_led create error\n");
	}
	if (pthread_create(&threads[1], NULL, &thread_verify, NULL) == -1) {
		//writeLogV2(LOGPATH, NAME, "thread_led create error\n");
	}
	// if (pthread_create(&threads[2], NULL, &tmr_exit, NULL) == -1) {
	// 	//writeLogV2(LOGPATH, NAME, "thread_led create error\n");
	// }
	

	event = redisInitialize();

	memset(&gConfig, 0, sizeof(Configuration));	// add 170905 for ini parser
	if (ini_parse("settings.ini", handler, &gConfig) < 0) {
		printf("Can't load 'settings.ini'\n");
		return 1;
	}
	printf(
			"Config loaded from 'settings.ini': tofmin=%d, tofmax=%d, toofar=%d, tooclose=%d \n",
			gConfig.tofmin, gConfig.tofmax, gConfig.toofar, gConfig.tooclose);

	//tof i2c init
	while (tof_open_return == TOF_OPEN_FAIL)
	{
		digitalWrite 	(XSHUT, HIGH);	
		digitalWrite 	(XSHUT, LOW);	usleep(1000*300);
		digitalWrite 	(XSHUT, HIGH);	usleep(1000*500);

		tof_open_return = tof_open(pMyDevice);

		if(tof_open_return == TOF_OPEN_FAIL)
		{
			printf("tof init fail \n\r");
		}
	}

	bio.state_curr = DETECT;

	int tof_init_cal_status;
	tof_init_cal_status = tof_init_cal_data(pMyDevice);

	int flag_detect = 0;
	int outCount = 0;
	int exitCount = 0;

	while (tof_init_cal_status == VL53L0X_ERROR_NONE) {

		tof_value = tof_get_cal_data(1, pMyDevice);
		if (tof_value > 9000) {
			printf("tof getting error(value = %d) -> exit \n\r", tof_value);
			tof_close(pMyDevice);
			exit(1);
			break;
		}
		else
		{
			
			if (tof_value > gConfig.tofmin && tof_value < gConfig.tofmax)
			{
				printf("DETECT %d \n\r ", tof_value);
				flag_detect = 1;
				outCount = 0;
				g_in_out = 1;
				sned_Tof_Value(tof_value, 1);
				exitCount = 0;
				exit_tmr_count = 0;
				usleep(1000 * 500);
			}
			// else if ( (tof_value < gConfig.tofmin || tof_value > gConfig.tofmax) && flag_detect == 1)
			// {
			// 	if(outCount == 0)
			// 	{
			// 		printf("OUT %d \n\r ", tof_value);
			// 		outCount = 1;
			// 		g_in_out = 2;
			// 	}
			// 	flag_detect = 0;
			// 	exitCount = 0;
			// }
			else
			{
				printf("exit count = %d , tof value = %d \n\r", exitCount, tof_value);
			}
			exitCount++;
		}
		if(exitCount == 750)	// count 1 = 0.16sec 	// count 1875 = 5min // 375 = 1min
		{
			// char publishCMD[] =	{ "PUBLISH TEST TOF" };
			// redisCommand(event, publishCMD);
			exit(1);
			break;
		}

	}
	// VL53L0X_ResetDevice(pMyDevice);
	tof_close_cal_data(pMyDevice);
	tof_close(pMyDevice);
	
	return 0;
}
void sned_Tof_Value(int value, int in_out)
{
	if( in_out == 1)
	{
		char pubStrig[40];
		memset(pubStrig, 0, sizeof(pubStrig));
		sprintf(pubStrig, "%s%d", "PUBLISH BACSModule verify;7;1;", value);
		printf("debug : %s \n\r", pubStrig);
		redisCommand(event, pubStrig);
	}
	else
	{
		char pubStrig[40];
		memset(pubStrig, 0, sizeof(pubStrig));
		sprintf(pubStrig, "%s%d", "PUBLISH BACSModule verify;7;0;", value);
		printf("debug : %s \n\r", pubStrig);
		redisCommand(event, pubStrig);
	}

}

#endif

