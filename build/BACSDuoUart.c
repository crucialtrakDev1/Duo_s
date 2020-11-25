/*
 ============================================================================
 Name        : main.c
 Author      : winston
 Version     :
 Copyright   : Your copyright notice
 ============================================================================
 */

#define bacs_duo_main_src


//-----------------------------------------------------------------------------------------------------------------------------------

#define STX 0x02
#define ETX 0x03

#ifdef bacs_duo_main_src

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

#include "tofM.h"
#include "tft_lcd_control.h"
#include "./lib/parser/ini.h"
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>
#include <wiringPi.h>

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

typedef struct
{
	const char* useRelay0;
	const char* useRelay1;
	int grantAccessTime0;
	int grantAccessTime1;
	int tofmin;
	int tofmax;
	int verifyVolume;
	int tamperEnable;
	int activeType;
	const char* modelType;
	int successCount;
	const char* useName;
	int wiegandBypass;
	const char* iotsecuritymode;
	int doorStatusActive;
} Configuration;



static int handler(void* config, const char* section, const char* name,
		const char* value)
{
	Configuration* pconfig = (Configuration*) config;

	if (MATCH("relay", "useRelay0"))
	{
		pconfig->useRelay0 = strdup(value);
	}
	else if (MATCH("relay", "useRelay1"))
	{
		pconfig->useRelay1 = strdup(value);
	}
	else if (MATCH("relay", "grantAccessTime0"))
	{
		pconfig->grantAccessTime0 = atoi(value);
	}
	else if (MATCH("relay", "grantAccessTime1"))
	{
		pconfig->grantAccessTime1 = atoi(value);
	}
	else if (MATCH("system", "tofmin"))
	{
		pconfig->tofmin = atoi(value);
	}
	else if (MATCH("system", "tofmax"))
	{
		pconfig->tofmax = atoi(value);
	}
	else if (MATCH("uart", "verifyVolume"))
	{
		pconfig->verifyVolume = atoi(value);
	}
	else if (MATCH("uart", "tamperEnable"))
	{
		pconfig->tamperEnable = atoi(value);
	}
	else if (MATCH("uart", "activeType"))
	{
		pconfig->activeType = atoi(value);
	}
	else if (MATCH("uart", "modelType"))
	{
		pconfig->modelType = strdup(value);
	}
	else if (MATCH("verify", "successCount"))
	{
		pconfig->successCount = atoi(value);
	}
	else if (MATCH("verify", "useName"))
	{
		pconfig->useName = strdup(value);
	}
	else if (MATCH("uart", "wiegandBypass"))
	{
		pconfig->wiegandBypass = atoi(value);
	}	
	else if (MATCH("verify", "iotsecuritymode"))
	{
		pconfig->iotsecuritymode = strdup(value);
	}		
	else if (MATCH("system", "doorStatusActive"))
	{
		pconfig->doorStatusActive = atoi(value);
	}


	else
	{
		return 0; /* unknown section/name, error */
	}
	return 1;
}

Configuration gConfig;

pthread_t threads[2];
redisContext *event;
static int fd; // Serial Port File Descriptor
int rdcnt;
char buf[50];
char serialBuf[50];
int global_jj_flag;
int global_jj_flag_cnt;
int fireAlarmStatus;
int global_door_cnt = 0;


int prev_door_status = 0;


void INIT_SOUND(int settingValue);
void JJ_CALL_FIRE_GATEOPEN(int activeCurent);
void ALIVE_CHK_JJ(void);
void door_close_tmr(void);
void door_close_tmr2(void);
void *thread_uart(void *);
void *redis_subscribe(void *arg);
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);
int strUpr(char *str)
{

	int loop = 0;
	while (str[loop] != '\0')
	{
		str[loop] = (char) toupper(str[loop]);
		loop++;
	}
	return loop;
}
redisContext* redisInitialize()
{

	redisContext* c;
	const char *hostname = "127.0.0.1";
	int port = 6379;
	struct timeval timeout =
	{ 1, 500000 }; // 1.5 seconds

	c = redisConnectWithTimeout(hostname, port, timeout);

	if (c == NULL || c->err)
	{
	}

	return c;

}
void *redis_subscribe(void *arg)
{
	struct event_base *base = event_base_new();
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
	if (c->err)
	{
		pthread_exit((void *) 0);
	}

	redisLibeventAttach(c, base);
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE BACSCore_GateOpen");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE Verify");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE duogpi");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE New_Icon");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE BACSDuoUart");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE GPIO_REV");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE Display");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE Enroll");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE IOT_GATEOPEN");
	redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE IOT_SECURITY");

	

	// redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE TEST");

	
	event_base_dispatch(base);
	exit(1);

	pthread_exit((void *) 0);
}
void *thread_uart(void *arg)
{
	//define for recive uart from JJ
	int bufCnt = 0;
	memset(buf, 0, sizeof(buf));
	memset(serialBuf, 0, sizeof(serialBuf));

	while (1)
	{		
		rdcnt = read(fd, buf, sizeof(buf));
		if (rdcnt > 0)		//start if((rdcnt > 0))
		{
			
			// printf("message = %s, length = %d \n\r", buf, rdcnt);
			for (int i = 0; i < rdcnt; i++)
			{
				if(buf[i] == STX)
				{
					printf("[DEBUG] reviced message length : %d \n\r", rdcnt);
				}
				else if(buf[i] == ETX)
				{
					printf("[JJ] received   	<-- %s \n\r", serialBuf);

					if(serialBuf[0] == 'e' && serialBuf[1] == 'r')		//case error
					{
						printf("Error : %s \n\r", serialBuf);
						exit(1);
					}
					else if(serialBuf[0] == 'w' && serialBuf[1] == 'i')		//case wiegand
					{
						if(gConfig.wiegandBypass == 1)
						{
							printf("message bypassed from wiegand input  \n\r");
							char wiegandBuffer[50];
							memset(wiegandBuffer, 0, sizeof(wiegandBuffer));
							sprintf(wiegandBuffer, "%s %s", "PUBLISH Wiegand_In", serialBuf );
							redisCommand(event, wiegandBuffer);
							printf("redis send : %s \n\r" , wiegandBuffer);									
						}
						if(gConfig.wiegandBypass == 2)
						{
							printf("wiegand input test mode  \n\r");
							char wiegandBuffer[50];
							memset(wiegandBuffer, 0, sizeof(wiegandBuffer));
							sprintf(wiegandBuffer, "%s %s", "PUBLISH TEST", serialBuf );
							redisCommand(event, wiegandBuffer);
							printf("redis send : %s \n\r" , wiegandBuffer);									
						}						
						else
						{
							printf("message forward to BACSModule from wiegand input  \n\r");
							char wiegandBuffer[50];
							memset(wiegandBuffer, 0, sizeof(wiegandBuffer));
							strcpy(wiegandBuffer, serialBuf);
							char *convertPtr;
							convertPtr = strtok( serialBuf, ";");
							if( convertPtr != NULL )
							{
								if (strcasecmp(convertPtr, "err") == 0)
								{

								}
								else
								{
									convertPtr = strtok( NULL, ";");
									if( convertPtr != NULL )
									{
										int cardType = atoi(convertPtr);
										printf("debugging : 1 : %s \n\r", convertPtr);
										printf("wiegand in card type(dec) : %d \n\r", cardType);

										convertPtr = strtok( NULL, ";");
										if( convertPtr != NULL )
										{
											long long card = strtoll(convertPtr, NULL, 16);
											printf("debugging : 2 : %s \n\r", convertPtr);
											printf("wiegand in card(dec) : %lld \n\r", card);
											char publishCMD[100];
											sprintf(publishCMD, "PUBLISH BACSModule verify;0;4;%lld", card);
											redisCommand(event, publishCMD);							
											printf("[redis] : %s \n\r", publishCMD );
										}										
									}									
								}
							}							
						}
					}
					else if (strcmp(serialBuf, "EX") == 0)		//Exit Low(active)
					{
						char publishCMD[] =	{ "PUBLISH duogpi 4;1;" };
						redisCommand(event, publishCMD);
						printf("Redis Publish : %s [%s] \n\r", publishCMD, serialBuf);
						

						global_door_cnt = gConfig.grantAccessTime0;


						printf("call gate open JJ active=%d, time=%d \n\r",	gConfig.activeType, gConfig.grantAccessTime0);

						char pubString[] = "";
						sprintf(pubString,"%s %s%d%s", "PUBLISH GPIO_SND", "{\"destination\":\"bacstestgpio\",\"key\":45,\"value\":", 1, "}");
						redisCommand(event, pubString);			
						printf("Redis Publish : %s \n\r", pubString);			
					}
					else if (strcmp(serialBuf, "EH") == 0)		//Exit high(default)
					{
						char publishCMD[] =	{ "PUBLISH duogpi 4;0;" };
						redisCommand(event, publishCMD);
						printf("Redis Publish : %s [%s] \n\r", publishCMD, serialBuf);

						char pubString[] = "";
						sprintf(pubString,"%s %s%d%s", "PUBLISH GPIO_SND", "{\"destination\":\"bacstestgpio\",\"key\":45,\"value\":", 0, "}");
						redisCommand(event, pubString);	
						printf("Redis Publish : %s \n\r", pubString);			
					}

					else if (strcmp(serialBuf, "DO") == 0)		//Door Low(active)
					{
						char publishCMD[] = { "PUBLISH duogpi 5;1;" };
						redisCommand(event, publishCMD);
						printf("Redis Publish : %s [%s] \n\r", publishCMD,	serialBuf);

						if(gConfig.doorStatusActive == 0)
						{
							char send_buf1[4] =	{ STX, 'N', 'O', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
						}
						else if(gConfig.doorStatusActive == 1)
						{
							char send_buf1[4] =	{ STX, 'N', 'C', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
						}
						else
						{
							char send_buf1[4] =	{ STX, 'N', 'O', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
						}						

						char pubString[] = "";
						sprintf(pubString,"%s %s%d%s", "PUBLISH GPIO_SND", "{\"destination\":\"bacstestgpio\",\"key\":43,\"value\":", 1, "}");
						redisCommand(event, pubString);	
						printf("Redis Publish : %s \n\r", pubString);
					}
					else if (strcmp(serialBuf, "DF") == 0)		//Door high(default)
					{
						char publishCMD[] =	{ "PUBLISH duogpi 5;0;" };
						redisCommand(event, publishCMD);
						printf("Redis Publish : %s [%s] \n\r", publishCMD, serialBuf);

						if(gConfig.doorStatusActive == 0)
						{
							char send_buf1[4] =	{ STX, 'N', 'C', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
						}
						else if(gConfig.doorStatusActive == 1)
						{
							char send_buf1[4] =	{ STX, 'N', 'O', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
						}
						else
						{
							char send_buf1[4] =	{ STX, 'N', 'C', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
						}
						

						char pubString[] = "";
						sprintf(pubString,"%s %s%d%s", "PUBLISH GPIO_SND", "{\"destination\":\"bacstestgpio\",\"key\":43,\"value\":", 0, "}");
						redisCommand(event, pubString);	
						printf("Redis Publish : %s \n\r", pubString);				
					}

					else if (strcmp(serialBuf, "FO") == 0)		//Fire Low(active)
					{
						char publishCMD[] =	{ "PUBLISH duogpi 6;1;" };
						redisCommand(event, publishCMD);
						printf("Redis Publish : %s [%s] \n\r", publishCMD,	serialBuf);

						char pubString[] = "";
						sprintf(pubString,"%s %s%d%s", "PUBLISH GPIO_SND", "{\"destination\":\"bacstestgpio\",\"key\":44,\"value\":", 1, "}");
						redisCommand(event, pubString);	
						printf("Redis Publish : %s \n\r", pubString);				
					}
					else if (strcmp(serialBuf, "FF") == 0)		//Fire High(default)
					{
						char publishCMD[] =	{ "PUBLISH duogpi 6;0;" };
						redisCommand(event, publishCMD);
						printf("Redis Publish : %s [%s] \n\r", publishCMD,	serialBuf);
						
						char pubString[] = "";
						sprintf(pubString,"%s %s%d%s", "PUBLISH GPIO_SND", "{\"destination\":\"bacstestgpio\",\"key\":44,\"value\":", 0, "}");
						redisCommand(event, pubString);		
						printf("Redis Publish : %s \n\r", pubString);			
					}
					else if (strcmp(serialBuf, "AO") == 0)		//Alram high(active)
					{
						char publishCMD[] =	{ "PUBLISH duogpi 7;1;" };
						redisCommand(event, publishCMD);
						printf("Redis Publish : %s [%s] \n\r", publishCMD, serialBuf);

						char pubString[] = "";
						sprintf(pubString,"%s %s%d%s", "PUBLISH GPIO_SND", "{\"destination\":\"bacstestgpio\",\"key\":46,\"value\":", 1, "}");
						redisCommand(event, pubString);		
						printf("Redis Publish : %s \n\r", pubString);			
					}
					else if (strcmp(serialBuf, "AF") == 0)	//Alram high(default)
					{
						char publishCMD[] =	{ "PUBLISH duogpi 7;0;" };	redisCommand(event, publishCMD);
						printf("Redis Publish : %s [%s] \n\r", publishCMD,	serialBuf);

						char pubString[] = "";
						sprintf(pubString,"%s %s%d%s", "PUBLISH GPIO_SND", "{\"destination\":\"bacstestgpio\",\"key\":46,\"value\":", 0, "}");
						redisCommand(event, pubString);		
						printf("Redis Publish : %s \n\r", pubString);

					}


					//io b'd start init
					else if (strcmp(serialBuf, "SU") == 0)
					{
						printf("JJ B'd Loading end \n\r");
						//step1 : set volume
						printf("Set verifyVolume \n\r");
						INIT_SOUND(gConfig.verifyVolume);
						//step2	: start sound
						char send_buf2[4] = {STX, 'S', 'U', ETX};	int rtn2 = write(fd, send_buf2, 4); usleep(1000 * 50);
						printf(" [JJ] send Response --> %s \n\r", send_buf2);

						redisReply *reply;
						reply = redisCommand(event,"GET securitymode");
						if(reply->str == NULL) reply->str = "0";
						printf("redis get securitymode : %s \n\r", reply->str);
						if (strcmp(reply->str, "1") == 0)	// '1' is security mode on
						{
							char send_buf[6] =	{ STX, 'O', 'N', 'S', 'O', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
						}
						else								// else (0, null) is security mode 'OFF'
						{
							char send_buf[6] =	{ STX, 'O', 'N', 'S', 'F', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
						}							
					}

					//io b'd alive check response
					else if (strcmp(serialBuf, "WD") == 0)
					{
						global_jj_flag = 1;
						char send_buf2[4] = {STX, 'W', 'D', ETX};	int rtn2 = write(fd, send_buf2, 4);
						printf(" [JJ] send Response --> %s \n\r ", serialBuf);
						char publishCMD[] =	{ "SET BACSDuoUart 1" };	redisCommand(event, publishCMD);	

					}
					else if (strcmp(serialBuf, "TO") == 0)
					{
						char wd_buf[2];
						int ret;

						ret = system("touch ~/tamperlog.txt");
						if (gConfig.tamperEnable == 1)
						{
							ret = system("sudo killall BACSModule Module.sh");

							printf(" [JJ] send Response --> %s \n\r ", wd_buf);
							//write redis tamper set
							char publishCMD[] =	{ "SET tamper 1" };	redisCommand(event, publishCMD);

							//delete excute
							char send_buf1[4] = {STX, 'L', 'F', ETX};	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
							char send_buf2[4] =	{ STX, 'S', 'F', ETX };		int rtn2 = write(fd, send_buf2, 4);	usleep(1000 * 50);
							char send_buf3[6] =	{ STX, 'O', 'N', 'C', 'O', ETX };	int rtn3 = write(fd, send_buf3, 6);	usleep(1000 * 50);
							char send_buf4[6] =	{ STX, 'O', 'N', 'A', 'O', ETX };	int rtn4 = write(fd, send_buf4, 6);	usleep(1000 * 50);

							char temperString[] = {"Tampering detected!"};
							int array_size = strlen(temperString) + 4;
							char send_string2[array_size];
							sprintf(send_string2, "%c%s%s%c", STX, "OP", temperString, ETX);
							int rtnN = write(fd, send_string2, array_size);
							printf("send alarm message : %s \n\r", send_string2);					

							for(int i=0; i<4; i++)
							{
								usleep(1000 * 500);
								int rtn1 = write(fd, send_buf1, 4);														usleep(1000 * 50);
								if (gConfig.verifyVolume != 0)
								{
									char send_buf3[4] =	{ STX, 'S', 'F', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
								}							
							}

							ret = system("/home/bacs/factorydefault/dataclear.sh");

							// write log
							printf("Tamper active!, delete database \n\r");
							ret = system("echo Tamper active!, delete database >> ~/tamperlog.txt");
							ret = system("date >> ~/tamperlog.txt");

							char publishTamper[] =	{ "PUBLISH BACSAlarm Tamper" };	redisCommand(event, publishTamper);	
						}
						else
						{
							//excute tamper don't delete data
							printf("Tamper active!, do not delete database \n\r");
							ret = system("echo Tamper active!, do not delete database >> ~/tamperlog.txt");
							ret = system("date >> ~/tamperlog.txt");
						}

					}
					else
					{
						printf("Undefined message 	: %s \n\r", serialBuf);
						printf("Message length 		: %d \n\r", (int)strlen(serialBuf));
					}
					// memset(buf, 0, sizeof(buf));
					bufCnt = 0;
					memset(serialBuf, 0, sizeof(serialBuf));
				}
				else
				{
					serialBuf[bufCnt] = buf[i];
					bufCnt++;					
				}
			}


		}
		else
		{
			memset(buf, 0, sizeof(buf));
		}
			//end if((rdcnt > 0))
	}	//end while(1)

		//stop thread (what kind if??)
//	pthread_exit((void *) 0);
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

		if (strcasecmp(r->element[0]->str, "message") == 0			//use test
		&& strcasecmp(r->element[1]->str, "duogpi") == 0)
		{
			if ((strcasecmp(r->element[2]->str, "DQ") == 0))
			{
				printf("<-- redis message DQ \n\r");
				char send_buf1[4] =	{ STX, 'D', 'Q', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
			}
			else
			{

			}

		}

		else if (strcasecmp(r->element[0]->str, "message") == 0	&& strcasecmp(r->element[1]->str, "GPIO_REV") == 0)
		{
			char gpio_key[20];
			char gpio_value[20];

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

			if		((strcasecmp(gpio_key, "38") == 0) && (strcasecmp(gpio_value, "true") == 0))
			{
				JJ_CALL_FIRE_GATEOPEN(1);
			}
			else if	((strcasecmp(gpio_key, "38") == 0) && (strcasecmp(gpio_value, "false") == 0))
			{
				JJ_CALL_FIRE_GATEOPEN(0);			
			}
		}



		else if (strcasecmp(r->element[0]->str, "message") == 0
						&& strcasecmp(r->element[1]->str, "TEST") == 0 )
		{
			strcpy(str, r->element[2]->str);
			ptr = strtok(str, ";");
			if( ptr == NULL ) return;
			if ((strcasecmp(ptr, "TOF") == 0))	// enroll start
			{
				char sendString[] = {"TOF RESTART"};
				int array_size = strlen(sendString) + 4;
				char send_string2[array_size];
				sprintf(send_string2, "%c%s%s%c", STX, "OG", sendString, ETX);
				int rtn2 = write(fd, send_string2, array_size);	usleep(1000 * 50);
				printf("debug : send string -> %s \n\r", send_string2);				

				char send_buf[4] =	{ STX, 'S', 'f', ETX };	int rtn = write(fd, send_buf, 4);	usleep(1000 * 50);
			}

			else
			{
				printf("undefined message : %s \n\r", r->element[2]->str);
			}
		}	




		else if (strcasecmp(r->element[0]->str, "message") == 0
						&& strcasecmp(r->element[1]->str, "Enroll") == 0 )
		{
			strcpy(str, r->element[2]->str);
			ptr = strtok(str, ";");
			if( ptr == NULL ) return;
			if ((strcasecmp(ptr, "1") == 0))	// enroll start
			{
				char sendString[] = {"Look at the camera"};
				int array_size = strlen(sendString) + 4;
				char send_string2[array_size];
				sprintf(send_string2, "%c%s%s%c", STX, "OG", sendString, ETX);
				int rtn2 = write(fd, send_string2, array_size);
				printf("debug : send string -> %s \n\r", send_string2);				
				
				// ptr = strtok(str, ";");	
				// if( ptr == NULL ) return;
				
				// if ((strcasecmp(ptr, "1") == 0))	//case face
				// {
				// 	char sendString[] = {"Look at the camera"};
				// 	int array_size = strlen(sendString) + 4;
				// 	char send_string2[array_size];
				// 	sprintf(send_string2, "%c%s%s%c", STX, "OG", sendString, ETX);
				// 	int rtn2 = write(fd, send_string2, array_size);
				// 	printf("debug : send string -> %s \n\r", send_string2);
				// }
			
				// else
				// {
				// 	printf("undefined case : %s \n\r", ptr);
				// }
			}
			else if ((strcasecmp(ptr, "3") == 0))	// enroll palm
			{

				// char sendString[] = {"Place your hand"};
				// int array_size = strlen(sendString) + 4;
				// char send_string2[array_size];
				// sprintf(send_string2, "%c%s%s%c", STX, "OG", sendString, ETX);
				// int rtn2 = write(fd, send_string2, array_size);
				// printf("debug : send string -> %s \n\r", send_string2);

				// ptr = strtok(str, ";");
				// if( ptr == NULL ) return;
				// if ((strcasecmp(ptr, "1") == 0))	//case face
				// {
				// 	char sendString[] = {"Look at the camera"};
				// 	int array_size = strlen(sendString) + 4;
				// 	char send_string2[array_size];
				// 	sprintf(send_string2, "%c%s%s%c", STX, "OG", sendString, ETX);
				// 	int rtn2 = write(fd, send_string2, array_size);
				// 	printf("debug : send string -> %s \n\r", send_string2);
				// }
			
				// else
				// {
				// 	printf("undefined case : %s \n\r", ptr);
				// }
			}



			else
			{
				printf("undefined message : %s \n\r", r->element[2]->str);
			}
		}			
		else if (strcasecmp(r->element[0]->str, "message") == 0
						&& strcasecmp(r->element[1]->str, "Display") == 0 )
		{
			strcpy(str, r->element[2]->str);
			ptr = strtok(str, ";");
			if( ptr == NULL ) return;
			if ((strcasecmp(ptr, "enroll") == 0))
			{
				ptr = strtok( NULL, ";");
				if( ptr == NULL ) return;
				if ((strcasecmp(ptr, "1") == 0))	//case face
				{
					ptr = strtok( NULL, ";");
					if( ptr == NULL ) return;
					if ((strcasecmp(ptr, "1") == 0))	//case face enroll sucess
					{
						char sendString[] = {"Captured"};
						int array_size = strlen(sendString) + 4;
						char send_string2[array_size];
						sprintf(send_string2, "%c%s%s%c", STX, "OG", sendString, ETX);
						int rtn2 = write(fd, send_string2, array_size);
						printf("debug : send string -> %s \n\r", send_string2);
					}
					else if ((strcasecmp(ptr, "0") == 0))	//case face enroll fail
					{
						char sendString[] = {"Failed"};
						int array_size = strlen(sendString) + 4;
						char send_string2[array_size];
						sprintf(send_string2, "%c%s%s%c", STX, "OG", sendString, ETX);
						int rtn2 = write(fd, send_string2, array_size);
						printf("debug : send string -> %s \n\r", send_string2);
					}
					else
					{
						printf("undefined result : %s \n\r", ptr);	
					}
				}
				else
				{
					printf("undefined case : %s \n\r", ptr);
				}
			}
			else
			{
				printf("undefined message : %s \n\r", r->element[2]->str);
			}
		}	
		else if (strcasecmp(r->element[0]->str, "message") == 0			
		&& strcasecmp(r->element[1]->str, "BACSDuoUart") == 0)
		{
			if ((strcasecmp(r->element[2]->str, "RESET") == 0))
			{
				char send_buf1[4] =	{ STX, 'H', 'R', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
			}
			else if ((strcasecmp(r->element[2]->str, "Settings") == 0))
			{
				usleep(1000 * 500);
				memset(&gConfig, 0, sizeof(Configuration));
				if (ini_parse("settings.ini", handler, &gConfig) < 0) {
					printf("Can't load 'settings.ini'\n");
					return ;
				}
				else
				{
					printf("changed settings.ini \n\r");
					printf("changed option success coount : %d \n\r", gConfig.successCount);
				}		
			}
			else
			{

			}
		}		
		else if (strcasecmp(r->element[0]->str, "message") == 0			
		&& strcasecmp(r->element[1]->str, "New_Icon") == 0)
		{
			if 	((strcasecmp(r->element[2]->str, "CRUAMS_ON") == 0))
			{
				printf("		redis message : %s \n\r", r->element[2]->str);
				char send_buf[6] =	{ STX, 'O', 'N', 'C', 'O', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
			}
			else if ((strcasecmp(r->element[2]->str, "CRUAMS_OFF") == 0))
			{
				// printf("		redis message : %s \n\r", r->element[2]->str);
				char send_buf[6] =	{ STX, 'O', 'N', 'C', 'F', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
			}
			else if ((strcasecmp(r->element[2]->str, "SECURITY_ON") == 0))
			{
				printf("		redis message : %s \n\r", r->element[2]->str);
				char send_buf[6] =	{ STX, 'O', 'N', 'S', 'O', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
			}
			else if ((strcasecmp(r->element[2]->str, "SECURITY_OFF") == 0))
			{
				printf("		redis message : %s \n\r", r->element[2]->str);
				char send_buf[6] =	{ STX, 'O', 'N', 'S', 'F', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
			}
			else if ((strcasecmp(r->element[2]->str, "ALARM_ON") == 0))
			{
				printf("		redis message : %s \n\r", r->element[2]->str);
				char send_buf[6] =	{ STX, 'O', 'N', 'A', 'O', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
			}		
			else if ((strcasecmp(r->element[2]->str, "ALARM_OFF") == 0))
			{
				printf("		redis message : %s \n\r", r->element[2]->str);
				char send_buf[6] =	{ STX, 'O', 'N', 'A', 'F', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
			}									
			else
			{

			}
		}		
		else if (strcasecmp(r->element[0]->str, "message") == 0
				&& strcasecmp(r->element[1]->str, "BACSCore_GateOpen") == 0)//if(gConfig.modelType == 4)
		{
			strcpy(str, r->element[2]->str);
//			printf("str : %s \n\r ", str);
			//redis string divide to redis_buf(type string)
			ptr = strtok(str, ";");
			if ((strcasecmp(ptr, "remote open") == 0))
			{					
				printf("call gate open JJ active=%d, time=%d \n\r",	gConfig.activeType, gConfig.grantAccessTime0);
				if(strcasecmp(gConfig.useRelay0, "false") == 0)
				{
					printf("remote open only");
					char send_bufH[4] = {STX, 'R', 'H', ETX};	int rtnH = write(fd, send_bufH, 4); //high
					usleep(1000* gConfig.grantAccessTime0);
					char send_bufL[4] = {STX, 'R', 'L', ETX};	int rtnL = write(fd, send_bufL, 4); //high					
				}
				else
				{
					global_door_cnt = gConfig.grantAccessTime0;
				}



				if (gConfig.verifyVolume != 0)
				{
					char send_buf3[4] =	{ STX, 'S', 'S', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
				}

				char send_buf1[4] = {STX, 'L', 'S', ETX};	int rtn1 = write(fd, send_buf1, 4);	//usleep(1000 * 50);

				char send_string2[20];
				sprintf(send_string2, "%c%s%s%c", STX, "OP", "DOOR OPEN", ETX);
				int rtn2 = write(fd, send_string2, sizeof(send_string2));
				printf("send matched name : %s \n\r", send_string2);
			}
			else if ((strcasecmp(ptr, "A") == 0))
			{
				JJ_CALL_FIRE_GATEOPEN(1);
				fireAlarmStatus = 1;
			}
			else if ((strcasecmp(ptr, "B") == 0))
			{
				JJ_CALL_FIRE_GATEOPEN(0);
				fireAlarmStatus = 0;
			}
		}

		else if (strcasecmp(r->element[0]->str, "message") == 0
						&& strcasecmp(r->element[1]->str, "IOT_SECURITY") == 0)					//if(gConfig.modelType == 4)
		{
			strcpy(str, r->element[2]->str);
			printf("str : %s \n\r ", str);
			//redis string divide to redis_buf(type string)
			ptr = strtok(str, ";");
			if( (strcasecmp(ptr, "CHECK") == 0) )
			{
				usleep(1000 * 100);	//sleep wait fot set
				redisReply *reply;
				reply = redisCommand(event,"GET securitymode");
				if(reply->str == NULL) reply->str = "0";
				printf("redis get securitymode : %s \n\r", reply->str);
				if (strcmp(reply->str, "1") == 0)	// '1' is security mode on
				{
					char send_buf[6] =	{ STX, 'O', 'N', 'S', 'O', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
				}
				else								// else (0, null) is security mode 'OFF'
				{
					char send_buf[6] =	{ STX, 'O', 'N', 'S', 'F', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
				}	
			}
			else
			{
				/* code */
			}
			
		}		
		else if (strcasecmp(r->element[0]->str, "message") == 0
						&& strcasecmp(r->element[1]->str, "IOT_GATEOPEN") == 0)					//if(gConfig.modelType == 4)
		{
			strcpy(str, r->element[2]->str);
			printf("str : %s \n\r ", str);
			//redis string divide to redis_buf(type string)
			ptr = strtok(str, ";");
			if( (strcasecmp(ptr, "OPEN") == 0) )
			{
				global_door_cnt = gConfig.grantAccessTime0;
			}
			else
			{
				/* code */
			}
			
		}			

		else if (strcasecmp(r->element[0]->str, "message") == 0
				&& strcasecmp(r->element[1]->str, "Verify") == 0)
		{

			strcpy(str, r->element[2]->str);
//			printf("str : %s \n\r ", str);
			//redis string divide to redis_buf(type string)
			ptr = strtok(str, ";");
			if ((strcasecmp(ptr, "start") == 0))
			{	//case "start;"
				char send_buf1[4] =	{ STX, 'L', 'D', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
				// char send_bufc[20];
				// memset(send_bufc, 0, sizeof(send_bufc));
				// sprintf(send_bufc, "%c%c%c%s%c", STX, 'L', 'C', "100100100", ETX); int rtnc = write(fd, send_bufc, 13);	usleep(1000*50);
				// printf("debug 190114: %s \n\r",send_bufc);					
				char send_buf2[6] =	{ STX, 'O', 'N', 'D', 'O', ETX };	int rtn2 = write(fd, send_buf2, 6);	usleep(1000 * 50);
				
			


				// redisReply *reply;
				// reply = redisCommand(event,"GET securitymode");
				// if(reply->str == NULL) reply->str = "0";
				// printf("redis get securitymode : %s \n\r", reply->str);
				// if (strcmp(reply->str, "1") == 0)	// '1' is security mode on
				// {
				// 	char send_buf[6] =	{ STX, 'O', 'N', 'S', 'O', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
				// }
				// else								// else (0, null) is security mode 'OFF'
				// {
				// 	char send_buf[6] =	{ STX, 'O', 'N', 'S', 'F', ETX };	int rtn = write(fd, send_buf, 6);	usleep(1000 * 50);
				// }				


			}
			else if ((strcasecmp(ptr, "stop") == 0))
			{	//case "stop;"
				char send_buf1[4] =	{ STX, 'L', 'W', ETX };	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
				char send_buf2[4] =	{ STX, 'O', 'T', ETX };	int rtn2 = write(fd, send_buf2, 4);	usleep(1000 * 50);
				char send_buf3[6] =	{ STX, 'O', 'N', 'D', 'F', ETX };	int rtn3 = write(fd, send_buf3, 6);	usleep(1000 * 50);
				printf("send status : stop \n\r");
			}


			else if ((strcasecmp(ptr, "result") == 0))
			{
				if (ptr != NULL)
				{
					ptr = strtok(NULL, ";");
					if ((strcasecmp(ptr, "3") == 0))		//palm
					{
						if (ptr != NULL)
						{
							ptr = strtok(NULL, ";");
							if ((strcasecmp(ptr, "0") == 0))//case "result;3;0;" palm fail
							{
								char send_string[11];
								sprintf(send_string, "%c%s%s%c", STX, "Op","fail", ETX);
								int rtn1 = write(fd, send_string, 11);

								if (gConfig.verifyVolume != 0)
								{
									if(gConfig.successCount <= 1)
									{
										//single mode each sound off
									}
									else
									{
										char send_buf3[4] =	{ STX, 'S', 'f', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
									}
								}
							}
							else	// palm success
							{
								char send_string[11];
								sprintf(send_string, "%c%s%s%c", STX, "Op","success", ETX);
								int rtn1 = write(fd, send_string, 11);

								if (gConfig.verifyVolume != 0)
								{
									if(gConfig.successCount <= 1)
									{
										//single mode each sound off

									}
									else
									{
										char send_buf3[4] =	{ STX, 'S', 's', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
									}

								}

							}
								char send_string[11];
								sprintf(send_string, "%c%s%s%c", STX, "OP","NULL", ETX);
								int rtn1 = write(fd, send_string, 11);

						}
					}
					else if ((strcasecmp(ptr, "1") == 0))		//face
					{
						if (ptr != NULL)
						{
							ptr = strtok(NULL, ";");
							if ((strcasecmp(ptr, "0") == 0))//case "result;1;0;" face fail
							{
//								char send_buf1[4] = {STX, 'L', 'F', ETX};	int rtn1 = write(fd, send_buf1, 4);	usleep(1000*100);

								if (gConfig.verifyVolume != 0)
								{
									if(gConfig.successCount <= 1)
									{
										//single mode each sound off
									}
									else
									{
										char send_buf3[4] =	{ STX, 'S', 'f', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
									}
								}
							}

							else	// face success
							{
								char send_string[11];
								sprintf(send_string, "%c%s%s%c", STX, "Of","success", ETX);
								int rtn1 = write(fd, send_string, 11);

								if (gConfig.verifyVolume != 0)
								{
									if(gConfig.successCount <= 1)
									{
										//single mode each sound off

									}
									else
									{
										char send_buf3[4] =	{ STX, 'S', 's', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
									}

								}
							}
						}
					}

					else if ((strcasecmp(ptr, "4") == 0))		//card
					{
						if (ptr != NULL)
						{
							ptr = strtok(NULL, ";");
							if ((strcasecmp(ptr, "0") == 0))//case "result;4;0;" card success
							{
//								char send_buf1[4] = {STX, 'L', 'F', ETX};	int rtn1 = write(fd, send_buf1, 4);	usleep(1000*100);

								if (gConfig.verifyVolume != 0)
								{
									if(gConfig.successCount <= 1)
									{
										//single mode each sound off
									}
									else
									{
										char send_buf3[4] =	{ STX, 'S', 'f', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
									}
								}
							}
							else	// card fail
							{
								char send_string[11];
								sprintf(send_string, "%c%s%s%c", STX, "Oc","success", ETX);
								int rtn1 = write(fd, send_string, 11);

								if (gConfig.verifyVolume != 0)
								{
									if(gConfig.successCount <= 1)
									{
										//single mode each sound off

									}
									else
									{
										char send_buf3[4] =	{ STX, 'S', 's', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
									}
								}
							}
						}
					}
					else
					{
					}	//not todo
				}
				else
				{
				}	//not todo
			}
			else if ((strcasecmp(ptr, "photo") == 0))
			{

				ptr = strtok( NULL, ";");
				if (ptr == NULL)
					return;

				ptr = strtok( NULL, ";");
				if (ptr == NULL)
					return;

				ptr = strtok( NULL, ";");
				if (ptr == NULL)
					return;

				if (gConfig.verifyVolume != 0)
				{
					char send_buf3[4] =	{ STX, 'S', 'S', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
				}

				char send_buf1[4] = {STX, 'L', 'S', ETX};	int rtn1 = write(fd, send_buf1, 4);	//usleep(1000 * 50);

				printf("gConfig.useName = %s \n\r", gConfig.useName);
				if( strcmp("true", gConfig.useName) != 0 )	//case false
				{
					int array_size = 4;
					char send_string2[array_size];
					sprintf(send_string2, "%c%s%c", STX, "OP", ETX);
					int rtn2 = write(fd, send_string2, array_size);
				}
				else										//case true
				{
					int array_size = strlen(ptr) + 4;
					char send_string2[array_size];
					sprintf(send_string2, "%c%s%s%c", STX, "OP", ptr, ETX);
					int rtn2 = write(fd, send_string2, array_size);
					printf("send matched name : %s \n\r", send_string2);
				}					

				if (strcmp(gConfig.iotsecuritymode, "true") == 0)
				{
					usleep(1000 * 1000);

					redisReply *reply;
					reply = redisCommand(event,"GET securitymode");
					if(reply->str == NULL) reply->str = "0";
					printf("redis get securitymode : %s \n\r", reply->str);
					if (strcmp(reply->str, "1") == 0)	// '1' is security mode on
					{
						//Don't open door when security mode 'ON'
					}
					else								// else (0, null) is security mode 'OFF'
					{
						printf("call gate open JJ active=%d, time=%d \n\r",	gConfig.activeType, gConfig.grantAccessTime0);
						global_door_cnt = gConfig.grantAccessTime0;
					}						
				}
				else
				{
					redisReply *reply;
					reply = redisCommand(event,"GET securitymode");
					if(reply->str == NULL) reply->str = "0";
					printf("redis get securitymode : %s \n\r", reply->str);
					if (strcmp(reply->str, "1") == 0)	// '1' is security mode on
					{
						//Don't open door when security mode 'ON'
					}
					else								// else (0, null) is security mode 'OFF'
					{
						printf("call gate open JJ active=%d, time=%d \n\r",	gConfig.activeType, gConfig.grantAccessTime0);
						global_door_cnt = gConfig.grantAccessTime0;
					}
				}
			}

			else if ((strcasecmp(ptr, "fail") == 0))
			{
				if (gConfig.verifyVolume != 0)
				{
					char send_buf3[4] =	{ STX, 'S', 'F', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
				}

				char send_buf1[4] = {STX, 'L', 'F', ETX};	int rtn1 = write(fd, send_buf1, 4);	usleep(1000 * 50);
				char send_buf2[4] = {STX, 'O', 'F', ETX};	int rtn2 = write(fd, send_buf2, 4);	usleep(1000 * 50);

				// char send_string[21];
				// sprintf(send_string, "%c%s%s%c", STX, "OG","Authentication failed", ETX);
				// int rtn2 = write(fd, send_string, 21);
			}
			else if ((strcasecmp(ptr, "guide") == 0))
			{
				ptr = strtok( NULL, ";");
				if (ptr == NULL)
					return;
				if ((strcasecmp(ptr, "reboot...") == 0))
				{
					printf("network reset ! \n\r");
					char send_buf2[4] =	{ STX, 'O', 'C', ETX };		int rtn2 = write(fd, send_buf2, 4);	usleep(1000 * 50);
					char send_buf3[4] =	{ STX, 'L', 'W', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
					char send_buf4[4] =	{ STX, 'N', '1', ETX };		int rtn4 = write(fd, send_buf4, 4);	usleep(1000 * 50);
				}
				else if ((strcasecmp(ptr, "network_reset") == 0))
				{
					printf("network reset ! \n\r");
					char send_buf2[4] =	{ STX, 'O', 'C', ETX };		int rtn2 = write(fd, send_buf2, 4);	usleep(1000 * 50);
					char send_buf3[4] =	{ STX, 'L', 'W', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
					char send_buf4[4] =	{ STX, 'N', '2', ETX };		int rtn4 = write(fd, send_buf4, 4);	usleep(1000 * 50);
				}
				else if ((strcasecmp(ptr, "factory_default") == 0))
				{
					printf("network reset ! \n\r");
					char send_buf2[4] =	{ STX, 'O', 'C', ETX };		int rtn2 = write(fd, send_buf2, 4);	usleep(1000 * 50);
					char send_buf3[4] =	{ STX, 'L', 'W', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);
					char send_buf4[4] =	{ STX, 'N', '3', ETX };		int rtn4 = write(fd, send_buf4, 4);	usleep(1000 * 50);
				}								
				else if ((strcasecmp(ptr, "palm") == 0)) 
				{
					ptr = strtok( NULL, ";");
					if (ptr == NULL)
						return;
					if ((strcasecmp(ptr, "99") == 0))
					{
						char send_string[11];
						sprintf(send_string, "%c%s%s%c", STX, "Op","success", ETX);
						int rtn1 = write(fd, send_string, 11);			

						char send_buf2[4] =	{ STX, 'S', 's', ETX };		int rtn2 = write(fd, send_buf2, 4);	usleep(1000 * 50);		
						if( strcmp("true", gConfig.modelType) != 0 )	//suite
						{
							char send_buf3[4] =	{ STX, 'L', 'B', ETX };		int rtn3 = write(fd, send_buf3, 4);	usleep(1000 * 50);		
						}
					}
				}
				else
				{
					int array_size = strlen(ptr) + 4;
					char send_string2[array_size];
					sprintf(send_string2, "%c%s%s%c", STX, "OG", ptr, ETX);
					int rtn2 = write(fd, send_string2, array_size);
					printf("debug : send guide -> %s \n\r", send_string2);
				}

			}
			else
			{
				
			}	//not todo
		}
		// else if (strcasecmp(r->element[0]->str, "message") == 0
		// 		&& strcasecmp(r->element[1]->str, "tempTof") == 0)//if(gConfig.modelType == 4)
		// {
		// 	strcpy(str, r->element[2]->str);
		// 	ptr = strtok(str, ";");
		// 	if ((strcasecmp(ptr, "1") == 0))
		// 	{
		// 		printf("Tof xshut high \n\r");
		// 		char send_buf4[4] =	{ STX, 'X', '1', ETX };		int rtn4 = write(fd, send_buf4, 4);	usleep(1000 * 50);

		// 	}
		// 	else
		// 	{
		// 		printf("Tof xshut low \n\r");
		// 		char send_buf4[4] =	{ STX, 'X', '0', ETX };		int rtn4 = write(fd, send_buf4, 4);	usleep(1000 * 50);
		// 	}
		// }
		else
		{

		} //not todo
	}
}
int open_serial(char *dev_name, int baud, int vtime, int vmin)
{
	int fd;
	struct termios newtio;

	fd = open(dev_name, O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		printf("Device  OPEN  FAIL  %s\n", dev_name);
		return -1;
	}
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR;
	//  non-parity  
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD;  // NO-rts/cts 
	switch (baud)
	{
	case 115200:
		newtio.c_cflag |= B115200;
		break;
	case 57600:
		newtio.c_cflag |= B57600;
		break;
	case 38400:
		newtio.c_cflag |= B38400;
		break;
	case 19200:
		newtio.c_cflag |= B19200;
		break;
	case 9600:
		newtio.c_cflag |= B9600;
		break;
	case 4800:
		newtio.c_cflag |= B4800;
		break;
	case 2400:
		newtio.c_cflag |= B2400;
		break;
	default:
		newtio.c_cflag |= B115200;
		break;
	}
	//set input mode (non-canonical, no echo,.....) 
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = vtime;    //  timeout  0.1
	newtio.c_cc[VMIN] = vmin;      //
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	/*---------------------------------------- Controlling RTS and DTR Pins --------------------------------------*/
	int DTR_flag;
	DTR_flag = TIOCM_DTR; /* Modem Constant for DTR pin */
	ioctl(fd, TIOCMBIS, &DTR_flag); /* setting DTR = 1,~DTR = 0 */
	ioctl(fd, TIOCMBIC, &DTR_flag); /* setting DTR = 0,~DTR = 1 */
	/*---------------------------------------- Controlling RTS and DTR Pins --------------------------------------*/

	return fd;
}
void close_serial(int fd)
{
	close(fd);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	printf("UART PROGRAM START \n\r");
	global_jj_flag = 0;
	global_jj_flag_cnt = 0;

	event = redisInitialize();


	memset( &gConfig, 0, sizeof( Configuration ) );
	if (ini_parse("settings.ini", handler, &gConfig) < 0) {
		printf("Can't load 'settings.ini'\n");
		return 1;
	}
	printf("Config load from 'settings.ini': verifyVolume : %d \n\r", 	gConfig.verifyVolume);
	printf("Config load from 'settings.ini': activeType : %d \n\r", 	gConfig.activeType);
	printf("Config load from 'settings.ini': grantAccessTime0 : %d \n\r", 	gConfig.grantAccessTime0);
	printf("Config load from 'settings.ini': modelType : %s \n\r", 	gConfig.modelType);
	printf("Config load from 'settings.ini': successCount : %d \n\r", 	gConfig.successCount);
	printf("Config load from 'settings.ini': wiegandBypass : %d \n\r", 	gConfig.wiegandBypass);
	


	fd = open_serial( "/dev/ttyUSBJJ", 9600, 1, 50 );
	if(fd == -1)
	{
		return 0;
	}




	//redis thread create ( Verify )
	if (pthread_create(&threads[0], NULL, &redis_subscribe, NULL) == -1)//verify
	{

	}

	if (pthread_create(&threads[1], NULL, &thread_uart, NULL) == -1)
	{

	}

	char publishCMD[] =	{ "SET BACSDuoUart 0" };	redisCommand(event, publishCMD);	

	while (1)
	{	//start while

		usleep(1000 * 100);	//100ms while process
		ALIVE_CHK_JJ();
		door_close_tmr();

	}	//end while

	close(fd);
	return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//not used
void INIT_SOUND(int settingValue)
{
	if (settingValue > 30)
	{
		settingValue = 30;
	}

	if (settingValue > 9)
	{
		int array_size = 6;
		char send_string[array_size];
		sprintf(send_string, "%c%s%d%c", STX, "IS", settingValue, ETX);
		int rtn1 = write(fd, send_string, array_size);	usleep(1000 * 50);
		printf("SOUND SETTING : str = %s, \n\r", send_string);
	}
	else
	{
		int array_size = 5;
		char send_string[array_size];
		sprintf(send_string, "%c%s%d%c", STX, "IS", settingValue, ETX);
		int rtn1 = write(fd, send_string, array_size);	usleep(1000 * 50);
		printf("SOUND SETTING : str = %s, \n\r", send_string);
	}
}

//void JJ_CALL_GATEOPEN(int activeTime, int activeType)
//{
//	if(fireAlarmStatus == 1)
//	{
//		printf("[DEBUG] : Can not open the door because fireAlarmStatus on now \n\r");
//		return ;
//	}
//
//	if(activeType == 1)	//case active high
//	{
//		printf("[DEBUG] : UP \n\r");
//		usleep(activeTime*1000);
//		char send_buf2[4] = {STX, 'R', 'L', ETX};	int rtn2 = write(fd, send_buf2, 4);
//		printf("[DEBUG] : DOWN \n\r");
//	}
//	else if(activeType == 0)	//case active low
//	{
//		char send_buf1[4] = {STX, 'R', 'L', ETX};	int rtn1 = write(fd, send_buf1, 4);
//		printf("[DEBUG] : DOWN \n\r");
//		usleep(activeTime*1000);
////		char send_buf1[2] = {'R', 'H'};	int rtn1 = write(fd, send_buf1, 2);	//high
//		char send_buf2[4] = {STX, 'R', 'H', ETX};	int rtn2 = write(fd, send_buf2, 4);
//		printf("[DEBUG] : UP \n\r");
//		usleep(1*1000);			//static 1 sec
//	}
//}
void JJ_CALL_FIRE_GATEOPEN(int activeCurent)
{
	if(activeCurent == 1)	//case active high (A)
	{
		char send_buf1[4] = {STX, 'R', 'H', ETX};	int rtn1 = write(fd, send_buf1, 4);
		printf("[DEBUG] : FIRE UP (%s) \n\r", send_buf1);
	}
	else if(activeCurent == 0)	//case active low (B)
	{
		char send_buf2[4] = {STX, 'R', 'L', ETX};	int rtn2 = write(fd, send_buf2, 4);
		printf("[DEBUG] : FIRE DOWN (%s) \n\r", send_buf2);
	}
}
void ALIVE_CHK_JJ(void)
{
	if (global_jj_flag == 0)
	{
		global_jj_flag_cnt++;
//		printf("temp debug : global_jj_flag_cnt : %d \n\r", global_jj_flag_cnt);

		if(global_jj_flag_cnt == 150)
		{
			global_jj_flag_cnt = 0;
			printf("Dead JJ & Open retry JJ \n\r");
			exit(1);

// 			//init JJ restart
// 			close(fd);
// 			fd = open_serial( "/dev/ttyUSBJJ", 9600, 1, 50 );
// 			if(fd == -1)
// 			{
// 				return;
// 			}

// 			printf("Config load from 'settings.ini': verifyVolume : %d \n\r", 	gConfig.verifyVolume);
// 			printf("Config load from 'settings.ini': activeType : %d \n\r", 	gConfig.activeType);
// 			printf("Config load from 'settings.ini': grantAccessTime0 : %d \n\r", 	gConfig.grantAccessTime0);
// 			printf("Config load from 'settings.ini': modelType : %s \n\r", 	gConfig.modelType);
// 			printf("JJ B'd Loading ... \n\r");
// //			sleep(1);
// 			//step1 : set volume
// 			printf("Set verifyVolume \n\r");
// 			INIT_SOUND(gConfig.verifyVolume);
// 			//step2	: start sound
// 			printf("Test out verifyVolume = %d \n\r", gConfig.verifyVolume);
// 			char send_buf1[4] = {STX, 'S', 'S', ETX};	int rtn1 = write(fd, send_buf1, 4); //usleep(1000 * 50);
// 			//init JJ end
		}
	}
	else if (global_jj_flag == 1)
	{
		//JJ alive don't anyting
		printf("receive WD alive count set 0  \n\r");
		global_jj_flag_cnt = 0;
		global_jj_flag = 0;
	}
	else
	{
		printf("come???  \n\r");
		//not todo
	}
}


void door_close_tmr(void)
{
//	printf("cnt : %d, prev : %d \n\r", global_door_cnt, prev_door_status);
	if(global_door_cnt <= 0)
	{
		if(prev_door_status == 0)
		{
			prev_door_status = 0;
		}
		else if (prev_door_status == 1)
		{
			char send_buf[4] = {STX, 'R', 'L', ETX};	int rtn2 = write(fd, send_buf, 4); //low
			printf("DEBUG : LOW, CNT : %d \n\r", global_door_cnt);
			prev_door_status = 0;
		}
		else
		{

		}
	}
	else
	{
		if(prev_door_status == 1)
		{
			prev_door_status = 1;
			global_door_cnt = global_door_cnt - 100;
		}
		else if (prev_door_status == 0)
		{
			if (strcasecmp(gConfig.useRelay0, "true") == 0 )
			{
				char send_buf[4] = {STX, 'R', 'H', ETX};	int rtn2 = write(fd, send_buf, 4); //high
				global_door_cnt = global_door_cnt - 100;
				printf("DEBUG : HIGH, CNT : %d \n\r", global_door_cnt);
			}
			prev_door_status = 1;
		}
		else
		{

		}
	}
}


#endif

			
