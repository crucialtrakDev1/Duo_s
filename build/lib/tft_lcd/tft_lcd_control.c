/*
 * tft_lcd_control.c
 *
 *  Created on: Aug 31, 2017
 *      Author: root
 */

#include "tft_lcd_control.h"

#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int lcd_open(tft_lcd *myLcdDevice)
{
	//보오율이나 stop bit 크기 등의 시리얼 통신 환경을 설정하기 위해
	// termios 구조체를 선언했습니다.
	struct termios newtio;

	// 통신 포트를 파일 개념으로 사용하기 위한 디스크립터 입니다.
	// 이 파일 디스크립터를 이용하여 표준 입출력 함수를 이용할 수 있습니다.

	myLcdDevice->fd = open( LCDDEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (myLcdDevice->fd < 0)
	{
		perror(LCDDEVICE);
		printf("return : %d \n", myLcdDevice->fd);
		return -1;
	}

	printf("SERIAL : %s\n", LCDDEVICE);
	// /dev/ttyS0를 사용하기 위해 open()함수를 사용합니다.
	// O_RDWR은 파일 디스크립터인 fd를 읽기와 쓰기 모드로 열기 위한 지정이며
	// O_NOCCTY와 O_NONBLOCK는 시리얼 통신 장치에 맞추어 추가했습니다.

	memset(&newtio, 0, sizeof(newtio));
	// 시리얼 통신환경 설정을 위한 구조체 변수 newtio 값을 0 바이트로 깨끗이 채웁니다.

	newtio.c_cflag = B115200;   // 통신 속도 115200
	//newtio.c_cflag |= CRTSCTS;   //futaba example add
	newtio.c_cflag |= CS8;      // 데이터 비트가 8bit
	newtio.c_cflag |= CLOCAL;   // 외부 모뎀을 사용하지 않고 내부 통신 포트 사용
	newtio.c_cflag |= CREAD;    // 쓰기는 기본, 읽기도 가능하게
	newtio.c_iflag = 0;         // parity 비트는 없음
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 1;

	tcflush(myLcdDevice->fd, TCIFLUSH);
	if (tcsetattr(myLcdDevice->fd, TCSANOW, &newtio) < 0)// 포트에 대한 통신 환경을 설정합니다.
	{
		perror(LCDDEVICE);
		printf("return : %d \n", myLcdDevice->fd);
		return -1;
	}

	// 표준 입출력 함수를 이용하여 시리얼 포트로 17자의 문자열을 전송합니다.
	return 1;
}

void serial_send(tft_lcd *myLcdDevice, uint8_t *buf, int bufSize)
{
	int w_return;
	w_return = write(myLcdDevice->fd, buf, bufSize);
//	printf("write byte = %d \n", w_return);
}

int send_string(tft_lcd *myLcdDevice, uint8_t *buf, int strLen, uint8_t pos_x,
		uint8_t pos_y, uint8_t color)	
{
	uint8_t buf3[] =
	{ 0x1B, 0x33, 0xFF, 0xFF };	// 0x33 --> DIGIT COLORS,  R 5bit, G 6bit, B 5bit

	// Color value 0x00 = white	= 0xFF, 0xFF
	// Color value 0x01 = RED	= 0xF8, 0x00
	// Color value 0x02 = GREEN	= 0x07, 0xE0
	// Color value 0x03 = BLUE	= 0x00, 0x1F
	switch(color)
	{
		case 0x00 : //White
		{
			buf3[2] = 0xff;  buf3[3] = 0xff;
			break;
		}
		case 0x01 : //Red
		{
			buf3[2] = 0xf8;  buf3[3] = 0x00;
			break;
		}
		case 0x02 : //Blue
		{
			buf3[2] = 0x07;  buf3[3] = 0xE0;
			break;
		}
		case 0x03 : //Green
		{
			buf3[2] = 0x00;  buf3[3] = 0x1F;
			break;
		}
		default :
		{
			break;
		}
	}


	//0x1B, 0xXX(CMD), 	0xXX(VALUE)
	uint8_t buf1[] =
	{ 0x1B, 0x35, 0x00 };// 0x35 --> CHARACTER ORNAMENT SETUP ,0x00 --> No character ornament
	uint8_t buf2[] =
	{ 0x1B, 0x32, 0x00 };// 0x32 --> TEXT SIZE SETTING, 0x00 --> 24x24pixel, 0x00 --> 24x24pixel
//	uint8_t buf3[] =
//	{ 0x1B, 0x33, 0xFF, 0xFF };	// 0x33 --> DIGIT COLORS,  R 5bit, G 6bit, B 5bit
	uint8_t buf4[] =
	{ 0x1B, 0x34, 0x01, 0x00, 0x00 };// 0x34 --> THE SPECIFICATION OF CHARACTER BACKGROUND COLOR, PDF Doc 35/69
	uint8_t buf5[] =
	{ 0x1B, 0x30, 0x00, pos_x, 0x00, pos_y };// 0x30 --> SETTING OF CURSOR POSITION, {0x1B, 0x30, High_X, Low_X , High_Y, Low_Y};
	uint8_t buf6[strLen];
	memcpy(buf6, buf, strLen);

	serial_send(myLcdDevice, buf1, sizeof(buf1));
	serial_send(myLcdDevice, buf2, sizeof(buf2));
	serial_send(myLcdDevice, buf3, sizeof(buf3));
	serial_send(myLcdDevice, buf4, sizeof(buf4));
	serial_send(myLcdDevice, buf5, sizeof(buf5));
	serial_send(myLcdDevice, buf6, sizeof(buf6));

	return 0;
}

int image_send_to_memory(tft_lcd *myLcdDevice, image *write_image,
		uint8_t init_cmd)
{
	//data type JPEG or PARTS
	uint8_t image_type;
	if (write_image->lcdMemoryAddress == BACK_IMAGE)
		image_type = 0xA2;
	else
		image_type = 0xA3;

	uint8_t image_high_length = (write_image->size / 0x100) / 0x100;
	uint8_t image_middle_length = (write_image->size / 0x100) % 0x100;
	uint8_t image_low_length = write_image->size % 0x100;

//	printf("img size buffer : [%02x][%02x][%02x] \n", image_high_length, image_middle_length, image_low_length);

	uint8_t buf1[] = { 0x1A, 0xA0 };									//F_ROM REGISTER MODE SHIFT
	serial_send(myLcdDevice, buf1, sizeof(buf1));

	uint8_t buf2[] = { 0x1A, image_type, write_image->lcdMemoryAddress, image_high_length,
			image_middle_length, image_low_length };	//JPEG DATA REGISTRATION
	serial_send(myLcdDevice, buf2, sizeof(buf2));

//check data buffer
//	printf("inner data ->");
//	for (int i = 0; i < write_image->size; i++)
//	{
//		printf("%02X ", write_image->dataBuf[i]);
//	}
//	printf("\n");

	if (write_image->size < SERIAL_MAX_SEND_SIZE)
	{
		serial_send(myLcdDevice, write_image->dataBuf,sizeof(write_image->size));
		sleep(5);
	}
	else
	{
		int bufQuantity = write_image->size / SERIAL_MAX_SEND_SIZE;	//입력한 JPEG 크기가 MAX_SEND 사이즈보다 클때 사용하는 버의 개수
		int lastBufSize = write_image->size % SERIAL_MAX_SEND_SIZE;	//입력한 JPEG 크기가 MAX_SEND 사이즈보다 클때 마지막 버퍼의 사이즈

		bufQuantity = write_image->size / SERIAL_MAX_SEND_SIZE;
		lastBufSize = write_image->size % SERIAL_MAX_SEND_SIZE;

		uint8_t lcdSendBuf[bufQuantity][SERIAL_MAX_SEND_SIZE];
		uint8_t lcdSendBufLast[lastBufSize];

//		printf("bufferQ : %d, bufferLast : %d \n", bufQuantity, lastBufSize);

		int x = 0, y = 0;

		for (x = 0; x < bufQuantity; x++)
		{
			for (y = 0; y < SERIAL_MAX_SEND_SIZE; y++)
			{
				lcdSendBuf[x][y] = write_image->dataBuf[(x
						* SERIAL_MAX_SEND_SIZE) + y];
			}
			serial_send(myLcdDevice, &lcdSendBuf[x][0], SERIAL_MAX_SEND_SIZE);
			sleep(1);
		}
		for (x = 0; x < lastBufSize; x++)
		{
			lcdSendBufLast[x] = write_image->dataBuf[(bufQuantity
					* SERIAL_MAX_SEND_SIZE) + x];
		}
		serial_send(myLcdDevice, &lcdSendBufLast[0], lastBufSize);
		sleep(5);

//check send buffer
//
//				int i=0,j=0;
//
//				printf("send data  ->");
//
//				for(i = 0; i < bufQuantity; i++)
//				{
//					for(j = 0; j < SERIAL_MAX_SEND_SIZE; j++)
//					{
//						printf("%02X ", lcdSendBuf[i][j]);
//					}
//				}
//				for(i = 0; i < lastBufSize; i++)
//				{
//					printf("%02X ", lcdSendBufLast[i]);
//				}
	}
	return 1;
}

void image_memory_write(tft_lcd *myLcdDevice)
{
	// 	define image information
	//	-image struct name
	//	-image data buffer allocation
	//	-image set file address
	//	-image address in lcd momory
	//	-image buffer memory initialize & enter size

	/*memory map
	 * - 0x00 -> background
	 * - 0x01 -> icon_face_off
	 * - 0x02 -> icon_palm_off
	 * - 0x03 -> icon_face_ok
	 * - 0x04 -> icon_palm_ok
	 * - 0x05 -> icon_face_fail	// don't write memory (don't use now)
	 * - 0x06 -> icon_palm_fail	// don't write memory (don't use now)
	 * - 0x07 -> icon_welcome	// don't write memory (don't use now)
	 * - 0x08 -> icon_face_detect
	 * - 0x09 -> icon_palm_detect
	 */

	uint8_t sw_resetBuf[] =
	{ 0x1B, 0x0B };							//software reset command
	serial_send(myLcdDevice, &sw_resetBuf[0], sizeof(sw_resetBuf));

	image main_background;
	main_background.fileAddress = "./jpeg/background.jpg";//static image
	main_background.dataBuf = (uint8_t*) malloc(
			sizeof(uint8_t) * TFT_JPEG_MAX_SIZE);
	main_background.lcdMemoryAddress = 0x00;
	if (jpeg_to_binary(&main_background))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
	{
		image_send_to_memory(myLcdDevice, &main_background, 1);
	}
	free(main_background.dataBuf);

	image parts_face_wait;
	parts_face_wait.fileAddress = "./jpeg/face_off_con.jpg";//static image
	parts_face_wait.dataBuf = (uint8_t*) malloc(
			sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
	parts_face_wait.lcdMemoryAddress = 0x01;
	if (jpeg_to_binary(&parts_face_wait))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
	{
		image_send_to_memory(myLcdDevice, &parts_face_wait, 1);
	}
	free(parts_face_wait.dataBuf);

	image parts_palm_wait;
	parts_palm_wait.fileAddress = "./jpeg/palm_off_con.jpg";//static image
	parts_palm_wait.dataBuf = (uint8_t*) malloc(
			sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
	parts_palm_wait.lcdMemoryAddress = 0x02;
	if (jpeg_to_binary(&parts_palm_wait))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
	{
		image_send_to_memory(myLcdDevice, &parts_palm_wait, 0);
	}
	free(parts_palm_wait.dataBuf);

	image parts_face_ok;
	parts_face_ok.fileAddress = "./jpeg/face_on_con.jpg";	//static image
	parts_face_ok.dataBuf = (uint8_t*) malloc(
			sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
	parts_face_ok.lcdMemoryAddress = 0x03;
	if (jpeg_to_binary(&parts_face_ok))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
	{
		image_send_to_memory(myLcdDevice, &parts_face_ok, 0);
	}
	free(parts_face_ok.dataBuf);

	image parts_palm_ok;
	parts_palm_ok.fileAddress = "./jpeg/palm_on_con.jpg";	//static image
	parts_palm_ok.dataBuf = (uint8_t*) malloc(
			sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
	parts_palm_ok.lcdMemoryAddress = 0x04;
	if (jpeg_to_binary(&parts_palm_ok))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
	{
		image_send_to_memory(myLcdDevice, &parts_palm_ok, 0);
	}
	free(parts_palm_ok.dataBuf);

//	palm fail image file not ready
//	image parts_palm_fail;
//	parts_palm_fail.fileAddress = "./src/jpeg/PALM_ON_L_con.jpg";	//static image
//	parts_palm_fail.dataBuf = (uint8_t*)malloc(sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
//	parts_palm_fail.lcdMemoryAddress = 0x05;
//	if(jpeg_to_binary(&parts_palm_fail))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
//	{
//		image_send_to_memory(myLcdDevice, &parts_palm_fail, 0);
//	}
//	free(parts_palm_fail.dataBuf);

	//face fail image file not ready
//	image parts_face_fail;
//	parts_face_fail.fileAddress = "./src/jpeg/PALM_ON_L_con.jpg";	//static image
//	parts_face_fail.dataBuf = (uint8_t*)malloc(sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
//	parts_face_fail.lcdMemoryAddress = 0x06;
//	if(jpeg_to_binary(&parts_face_fail))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
//	{
//		image_send_to_memory(myLcdDevice, &parts_face_fail, 0);
//	}
//	free(parts_face_fail.dataBuf);

//	image parts_welcome;
//	parts_welcome.fileAddress = "./jpeg/welcome_con.jpg";	//static image
//	parts_welcome.dataBuf = (uint8_t*) malloc(
//			sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
//	parts_welcome.lcdMemoryAddress = 0x07;
//	if (jpeg_to_binary(&parts_welcome))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
//	{
//		image_send_to_memory(myLcdDevice, &parts_welcome, 0);
//	}
//	free(parts_welcome.dataBuf);

	image parts_tof_detect_face;
	parts_tof_detect_face.fileAddress = "./jpeg/detect_face_con.jpg";	//static image
	parts_tof_detect_face.dataBuf = (uint8_t*) malloc(
			sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
	parts_tof_detect_face.lcdMemoryAddress = 0x08;
	if (jpeg_to_binary(&parts_tof_detect_face))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
	{
		image_send_to_memory(myLcdDevice, &parts_tof_detect_face, 0);
	}
	free(parts_tof_detect_face.dataBuf);

	image parts_tof_detect_palm;
	parts_tof_detect_palm.fileAddress = "./jpeg/detect_palm_con.jpg";	//static image
	parts_tof_detect_palm.dataBuf = (uint8_t*) malloc(
			sizeof(uint8_t) * TFT_PARS_MAX_SIZE);
	parts_tof_detect_palm.lcdMemoryAddress = 0x09;
	if (jpeg_to_binary(&parts_tof_detect_palm))	//데이터 버퍼와 데이터 사이즈를 입력 한다.
	{
		image_send_to_memory(myLcdDevice, &parts_tof_detect_palm, 0);
	}
	free(parts_tof_detect_palm.dataBuf);
}

void lcd_close(tft_lcd *myLcdDevice)
{
	close(myLcdDevice->fd);
	// 통신 포트를 닫아 사용을 중지합니다.
}

int jpeg_to_binary(image *imageBuf)
{
	FILE * src;
	int cn = 0;
	int buf_cnt = 0;

	src = fopen(imageBuf->fileAddress, "r+b");
	if (src == NULL)
	{
		puts("파일오픈 실패!");
		printf("%s fail target : %s \n", __func__, imageBuf->fileAddress);
	}

	while (1)
	{
		if (((cn = fgetc(src))) != EOF)
		{
			imageBuf->dataBuf[buf_cnt] = (unsigned char) cn;
			buf_cnt++;
		}
		else
		{
			break;
		}
	}
	fclose(src);
	imageBuf->size = buf_cnt - 6;

	printf("%s ok | size : %d byte | target : %s \n", __func__, imageBuf->size,
			imageBuf->fileAddress);
	return 1;
}

void set_background_image(tft_lcd *myLcdDevice, uint8_t address)
{
	uint8_t buf1[] =
	{ 0x1A, 0xA1 };								//FROM REGISTER MODE RELEASE
	serial_send(myLcdDevice, &buf1[0], sizeof(buf1));
	uint8_t buf2[] =
	{ 0x1B, 0x21, address };					//Display of JPEG (Upper layer mode)
	serial_send(myLcdDevice, &buf2[0], sizeof(buf2));
	uint8_t buf3[] =
	{ 0x1B, 0x20, 0xFF };						//DIMMING SETUP 100%
	serial_send(myLcdDevice, &buf3[0], sizeof(buf3));

}

void set_parts_image(tft_lcd *myLcdDevice, uint8_t address, uint8_t pos_x,
		uint8_t pos_y)
{
	uint8_t buf1[] =
	{ 0x1B, 0x25, 0x00, pos_x, 0x00, pos_y, address };
	serial_send(myLcdDevice, &buf1[0], sizeof(buf1));
}
void set_parts_clear (tft_lcd *myLcdDevice)
{
	uint8_t buf1[] =
	{ 0x1B, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0x03, 0xFF };
	serial_send(myLcdDevice, &buf1[0], sizeof(buf1));
}
void set_string_clear (tft_lcd *myLcdDevice) 	//1Bh, 2Dh, Bx, By, Bw, Bh
{
	uint8_t buf1[] =
	{ 0x1B, 0x2D, 	0x00, 0x00, 	0x00, 0x00, 	0x08, 0x00, 	0x00, 0x35 };
	serial_send(myLcdDevice, &buf1[0], sizeof(buf1));
}
void set_guide_clear (tft_lcd *myLcdDevice) 	//1Bh, 2Dh, Bx, By, Bw, Bh
{
	uint8_t buf1[] =
	{ 0x1B, 0x2D, 	0x00, 0x05, 	0x00, 0xC5, 	0x03, 0xFF, 	0x00, 0xFF };
	serial_send(myLcdDevice, &buf1[0], sizeof(buf1));
}

