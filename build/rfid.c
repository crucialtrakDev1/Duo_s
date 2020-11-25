/*
 ============================================================================
 Name        : rfid.c
 Author      : winstonlee
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "rfid.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <termios.h>

#include "./lib/parser/ini.h"

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

typedef struct
{
	const char* cardType;
} Configuration;

static int handler(void* config, const char* section, const char* name,
		const char* value)
{
	Configuration* pconfig = (Configuration*) config;

	if (MATCH("card", "cardType"))
	{
		pconfig->cardType = strdup(value);
	}
	else
	{
		return 0; /* unknown section/name, error */
	}
	return 1;
}
Configuration gConfig;

redisContext *event;

int main(void)
{

	event = redisInitialize();

	int fd; // 시리얼포트 파일핸들
	int rdcnt;
	int bufCnt;

	printf(" device : /dev/ttyRFID 	\n\r"
			" baud : 115200 		\n\r"
			" not used RTS/CTS 		\n\r");

	fd = open_serial(LINUX_DEVICE, BAUD_RATE, 1, 30);
	if (fd < 0)
	{
		close_serial(fd);
		return -2;
	}

	memset( &gConfig, 0, sizeof( Configuration ) );
	if (ini_parse("settings.ini", handler, &gConfig) < 0) {
		printf("Can't load 'settings.ini'\n");
		return 1;
	}

	printf("Config load from 'settings.ini' -> RFID cardType : %s \n\r", 	gConfig.cardType);

	int receiveCounter = 0;
	char charBuf[1];
	char receiveBuffer[20];
	char pubString[50];

	memset(pubString, 0, 		sizeof(pubString));
	memset(charBuf, 0, sizeof(charBuf));
	memset(receiveBuffer, 0, sizeof(receiveBuffer));

	while (1)
	{
		int i;
		rdcnt = read(fd, charBuf, sizeof(charBuf));



		if (rdcnt != 0)
		{

			printf("test : 0x%02X \n\r", charBuf[0]);


			if(charBuf[0] == 0x02)
			{
				printf("Received STX \n\r");
				receiveBuffer[receiveCounter] = charBuf[0];
				receiveCounter++;	
			}
			else if(charBuf[0] == 0x03)
			{
				receiveBuffer[receiveCounter] = charBuf[0];
				receiveCounter++;
				printf("Received ETX \n\r\n\r");
				printf("debug : %s \n\r", receiveBuffer);

				printf("conversion start \n\r");
				//card conversion
				if ( (strcmp(gConfig.cardType, "normal") == 0) || (strcmp(gConfig.cardType, "NORMAL") == 0) )
				{
					char convertBuf[10];
					unsigned long decBuf;
					char intStrBuf[10];

					printf("saved cardType = %s \n\r", gConfig.cardType);
					printf("HEX(8) -> DEC string \n\r");

					if(receiveBuffer[9] != 0x03)
					{
						printf("parsing error \n\r");
						return 0 ;	
					} 	

					sprintf(convertBuf, "%c%c%c%c%c%c%c%c", 
							receiveBuffer[1], receiveBuffer[2], receiveBuffer[3], receiveBuffer[4], 
							receiveBuffer[5], receiveBuffer[6], receiveBuffer[7], receiveBuffer[8]);	


					decBuf = strtoul(convertBuf, NULL, 16);
					printf("debug dec = %lu \n\r", (long unsigned int)decBuf);
					sprintf(intStrBuf, "%lu", (long unsigned int)decBuf);	// no 0 fill
					

					// decBuf = strtoul(convertBuf, NULL, 16);
					// printf("debug dec = %10lu \n\r", (long unsigned int)decBuf);
					// sprintf(intStrBuf, "%10lu", (long unsigned int)decBuf);	//fill

					for(int i=0; i<10; i++)
					{
						printf("test : 0x%02x \n\r", intStrBuf[i]);
						if(intStrBuf[i] == 32)
						{
							intStrBuf[i] = 0x30;								
						}
						printf("test : %c \n\r", intStrBuf[i]);
							
					}

					sprintf(pubString, "PUBLISH BACSModule verify;0;4;%s;", intStrBuf);
					redisCommand(event, pubString);

					printf("Redis[OUT] = %s \n\r\n\r", pubString);


					memset(charBuf, 0, 			sizeof(charBuf));
					memset(receiveBuffer, 0, 	sizeof(receiveBuffer));
					memset(pubString, 0, 		sizeof(pubString));
					receiveCounter = 0;			
				}
				else if ( (strcmp(gConfig.cardType, "hid") == 0) || (strcmp(gConfig.cardType, "HID") == 0) )
				{
					char convertBuf[10];
					unsigned int decBuf;
					char intStrBuf[10];

					printf("saved cardType = %s \n\r", gConfig.cardType);
					printf("HEX(8) -> HID style DEC string \n\r");

					if(receiveBuffer[9] != 0x03)
					{
						printf("parsing error \n\r");
						return 0 ;	
					} 	
					sprintf(convertBuf, "%c%c%c%c%c%c%c%c", 
							receiveBuffer[7], receiveBuffer[8], receiveBuffer[5], receiveBuffer[6], 
							receiveBuffer[3], receiveBuffer[4], receiveBuffer[1], receiveBuffer[2]);

					decBuf = strtoul(convertBuf, NULL, 16);

					sprintf(intStrBuf, "%u", decBuf);

					//do publish
					char publishCMD[] =
					{ "PUBLISH BACSModule verify;0;4;" };
					strcat(publishCMD, intStrBuf);
					strcat(publishCMD, ";");
					redisCommand(event, publishCMD);

					//result print
					printf("Redis[OUT] = %s \n\r\n\r", publishCMD);

					memset(charBuf, 0, 			sizeof(charBuf));
					memset(receiveBuffer, 0, 	sizeof(receiveBuffer));
					memset(pubString, 0, 		sizeof(pubString));
					receiveCounter = 0;		
				}
				else if ( (strcmp(gConfig.cardType, "lg") == 0) || (strcmp(gConfig.cardType, "LG") == 0) )
				{
					char convertBuf_head[4];
					char convertBuf_tail[4];
					uint64_t decBuf;
					unsigned int decBuf_head;
					unsigned int decBuf_tail;
					char intStrBuf_head[5];
					char intStrBuf_tail[5];
					char intStrBuf[11];
					int zeroCnt = 0;					
					
					printf("saved cardType = %s \n\r", gConfig.cardType);
					printf("HEX(8) -> LG style DEC string \n\r");

					if(receiveBuffer[9] != 0x03)
					{
						printf("parsing error \n\r");
						return 0 ;	
					} 						

					sprintf(convertBuf_head, "%c%c%c%c",receiveBuffer[1], receiveBuffer[2], receiveBuffer[3], receiveBuffer[4]);

					sprintf(convertBuf_tail, "%c%c%c%c",receiveBuffer[5], receiveBuffer[6], receiveBuffer[7], receiveBuffer[8]);

					decBuf_head = strtoul(convertBuf_head, NULL, 16);
					decBuf_tail = strtoul(convertBuf_tail, NULL, 16);

					sprintf(intStrBuf, "%05d%05d" , decBuf_head, decBuf_tail);

					//do publish
					char publishCMD[] = {"PUBLISH BACSModule verify;0;4;"};
					strcat(publishCMD, intStrBuf);
					strcat(publishCMD, ";");
					redisCommand(event, publishCMD);

					//result print
					printf("Redis[OUT] = %s \n\r\n\r", publishCMD);

					memset(charBuf, 0, 			sizeof(charBuf));
					memset(receiveBuffer, 0, 	sizeof(receiveBuffer));
					memset(pubString, 0, 		sizeof(pubString));
					receiveCounter = 0;		

				}
				else if ( (strcmp(gConfig.cardType, "felica") == 0) || (strcmp(gConfig.cardType, "FELICA") == 0) )
				{
					printf("saved cardType = %s \n\r", gConfig.cardType);
					printf("HEX(16) -> Felica style HEX string \n\r");

					if(receiveBuffer[17] != 0x03)
					{
						printf("parsing error \n\r");
						return 0 ;	
					} 						
					char convertBuf[16];

					sprintf(convertBuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 
							receiveBuffer[1], receiveBuffer[2], receiveBuffer[3], receiveBuffer[4], receiveBuffer[5], receiveBuffer[6],
							receiveBuffer[7], receiveBuffer[8], receiveBuffer[9], receiveBuffer[10], receiveBuffer[11], receiveBuffer[12],
							receiveBuffer[13], receiveBuffer[14], receiveBuffer[15], receiveBuffer[16]);


					// unsigned long long decimalBuffer =  strtoul(convertBuf, NULL, 16);
					unsigned long long decimalBuffer =  (unsigned long long)strtoull(convertBuf, NULL, 16);
					printf("convert decimal = %llu \n\r ", decimalBuffer);
					char decStringBuffer[45];
					memset(decStringBuffer, 0, sizeof(decStringBuffer));
					sprintf(decStringBuffer, "%llu" , decimalBuffer);

					//do publish
					char publishCMD[] =
					{ "PUBLISH BACSModule verify;0;4;" };
					strcat(publishCMD, decStringBuffer);
					strcat(publishCMD, ";");
					redisCommand(event, publishCMD);

					//result print
					printf("Redis[OUT] = %s \n\r\n\r", publishCMD);

					memset(charBuf, 0, 			sizeof(charBuf));
					memset(receiveBuffer, 0, 	sizeof(receiveBuffer));
					memset(pubString, 0, 		sizeof(pubString));
					receiveCounter = 0;	
				}
				else if ( (strcmp(gConfig.cardType, "mifare_ultra_light") == 0) || (strcmp(gConfig.cardType, "MIFARE_ULTRA_LIGHT") == 0) )
				{
					int bufferSize = receiveCounter-2;
					char convertBuf[bufferSize];
					// unsigned long decBuf;
					unsigned long long decBuf;
					// uint64_t decBuf;
					char intStrBuf[20];

					printf("saved cardType = %s \n\r", gConfig.cardType);
										printf("testing....\n\r");

					printf("HEX(8) -> DEC string \n\r");

					printf("test 0x%02x \n\r", receiveBuffer[15]);

					if(receiveBuffer[15] != 0x03)
					{
						printf("parsing error \n\r");
						return 0 ;	
					} 	

					sprintf(convertBuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 
							receiveBuffer[1], receiveBuffer[2], receiveBuffer[3], receiveBuffer[4], receiveBuffer[5], receiveBuffer[6], receiveBuffer[7], 
							receiveBuffer[8], receiveBuffer[9], receiveBuffer[10], receiveBuffer[11], receiveBuffer[12], receiveBuffer[13], receiveBuffer[14]);	

					printf("HEX STRING : %s \n\r", convertBuf);


					decBuf = strtoull(convertBuf, NULL, 16);
					printf("debug dec = %llu \n\r", (long long unsigned) decBuf);
					sprintf(intStrBuf, "%llu", (long long unsigned ) decBuf);	// no 0 fill
					

					// decBuf = strtoul(convertBuf, NULL, 16);
					// printf("debug dec = %10lu \n\r", (long unsigned int)decBuf);
					// sprintf(intStrBuf, "%10lu", (long unsigned int)decBuf);	//fill

					// for(int i=0; i<16; i++)
					// {
					// 	printf("test : 0x%02x \n\r", intStrBuf[i]);
					// 	if(intStrBuf[i] == 32)
					// 	{
					// 		intStrBuf[i] = 0x30;								
					// 	}
					// 	printf("test : %c \n\r", intStrBuf[i]);
							
					// }

					sprintf(pubString, "PUBLISH BACSModule verify;0;4;%s;", intStrBuf);
					redisCommand(event, pubString);

					printf("Redis[OUT] = %s \n\r\n\r", pubString);


					memset(charBuf, 0, 			sizeof(charBuf));
					memset(receiveBuffer, 0, 	sizeof(receiveBuffer));
					memset(pubString, 0, 		sizeof(pubString));
					receiveCounter = 0;			
				}				
				else if ( (strcmp(gConfig.cardType, "komico") == 0) || (strcmp(gConfig.cardType, "KOMICO") == 0))
				{
					printf("saved cardType = %s \n\r", gConfig.cardType);
					printf("HEX(13) -> Komico style HEX string \n\r");

					if(receiveBuffer[14] != 0x03)
					{
						printf("parsing error \n\r");
						return 0 ;	
					} 

					char convertBuf[13];

					sprintf(convertBuf, "%c%c%c%c%c%c%c%c%c%c%c%c%c", 
							receiveBuffer[1], receiveBuffer[2], receiveBuffer[3], receiveBuffer[4], receiveBuffer[5], 
							receiveBuffer[6], receiveBuffer[7], receiveBuffer[8], receiveBuffer[9], receiveBuffer[10], 
							receiveBuffer[11],receiveBuffer[12],receiveBuffer[13]);

							

					//do publish
					char publishCMD[] =
					{ "PUBLISH BACSModule verify;0;4;" };
					strcat(publishCMD, convertBuf);
					strcat(publishCMD, ";");
					redisCommand(event, publishCMD);

					//result print
					printf(" Redis[OUT] = %s \n\r\n\r", publishCMD);

					memset(charBuf, 0, 			sizeof(charBuf));
					memset(receiveBuffer, 0, 	sizeof(receiveBuffer));
					memset(pubString, 0, 		sizeof(pubString));
					receiveCounter = 0;	
				}											
				else
				{
					printf("[warning] used default(normal)mode because don't set cardType \n\r");
					printf("[warning] saved cardType = %s \n\r", gConfig.cardType);
					printf("HEX(n) -> DEC string, length(n) = %d \n\r", receiveCounter);

					char convertBuf[10];
					unsigned int decBuf;
					char intStrBuf[10];


					if(receiveBuffer[9] != 0x03)
					{
						printf("parsing error \n\r");
						return 0 ;	
					} 	

					sprintf(convertBuf, "%c%c%c%c%c%c%c%c", 
							receiveBuffer[1], receiveBuffer[2], receiveBuffer[3], receiveBuffer[4], 
							receiveBuffer[5], receiveBuffer[6], receiveBuffer[7], receiveBuffer[8]);		

					decBuf = strtoul(convertBuf, NULL, 16);
					// sprintf(intStrBuf, "%lu", (long unsigned int)decBuf);	// no 0 fill
					sprintf(intStrBuf, "%10d", (unsigned int)decBuf);		// 0 fill
					for(int i=0; i<10; i++)
					{
						if(intStrBuf[i] == 32)
							intStrBuf[i] = 0x30;
					}


					sprintf(pubString, "PUBLISH BACSModule verify;0;4;%s;", intStrBuf);
					redisCommand(event, pubString);

					printf("Redis[OUT] = %s \n\r\n\r", pubString);


					memset(charBuf, 0, 			sizeof(charBuf));
					memset(receiveBuffer, 0, 	sizeof(receiveBuffer));
					memset(pubString, 0, 		sizeof(pubString));
					receiveCounter = 0;			
				}
			}
			else
			{
				receiveBuffer[receiveCounter] = charBuf[0];
				receiveCounter++;				
			}
		}
	}

	close_serial(fd);
	printf(" /dev/ttyRFID closed\n");

	return EXIT_SUCCESS;
}

int open_serial(char *dev_name, int baud, int vtime, int vmin)
{

	//------------------------------------------------------------------------------
	// 설명 : 시리얼포트를 연다.
	// 주의 : RTS/CTS 를 제어하지 않는다.
	// 시리얼포트를 열고 이전의 포트설정상태를 저장하지 않았다.
	//------------------------------------------------------------------------------

	int fd;
	struct termios newtio;
// 시리얼포트를 연다.
	fd = open(dev_name, O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		// 화일 열기 실패
		printf("Device OPEN FAIL %s\n", dev_name);
		return -1;
	}
// 시리얼 포트 환경을 설정한다.
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR; // non-parity
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts

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
	newtio.c_cc[VTIME] = vtime; // timeout 0.1초 단위
	newtio.c_cc[VMIN] = vmin; // 최소 n 문자 받을 때까진 대기
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	return fd;
}

void close_serial(int fd)
{

	//------------------------------------------------------------------------------
	// 설명 : 시리얼포트를 닫는다.
	// 주의 :
	//------------------------------------------------------------------------------

	close(fd);
}

redisContext* redisInitialize()
{
	redisContext* c;
	const char *hostname = "127.0.0.1";
	//const char *hostname = "192.168.100.110";
	int port = 6379;
	struct timeval timeout =
	{ 1, 500000 }; // 1.5 seconds

	c = redisConnectWithTimeout(hostname, port, timeout);

	if (c == NULL || c->err)
	{
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


