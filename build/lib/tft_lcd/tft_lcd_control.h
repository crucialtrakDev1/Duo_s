/*
 * tft_lcd_control.h
 *
 *  Created on: Aug 31, 2017
 *      Author: root
 */

#ifndef LIB_TFT_LCD_TFT_LCD_CONTROL_H_
#define LIB_TFT_LCD_TFT_LCD_CONTROL_H_

#include <stdint.h>


#define LCDDEVICE "/dev/ttyS0"	//duo
//#define LCDDEVICE "/dev/ttyUSBLCD"	//duo_s, pc

#define TFT_JPEG_MAX_SIZE		(262142)
#define TFT_PARS_MAX_SIZE		(65535)

#define BACK_IMAGE				0
#define PARTS_IMAGE				1

#define SERIAL_MAX_SEND_SIZE	4000

//state define
#define WAIT_WELCOME		0
#define DETECT				1

#define OFF					0
#define ON					1
//#define_FAIL				-1






typedef struct
{
	int   fd;

} tft_lcd;

typedef struct
{
	char *fileAddress;
	uint8_t *dataBuf;
	int	size;

	uint8_t lcdMemoryAddress;
} image;

typedef struct
{
	uint8_t face;
	uint8_t	palm;

	uint8_t state_curr;
	uint8_t state_prev;

	uint8_t	stringBuf[50];
	uint8_t receiveName[20];

	uint8_t open;

} bio_state;




int 	lcd_open					(tft_lcd *myLcdDevice);
void 	lcd_close					(tft_lcd *myLcdDevice);

void 	image_memory_write				(tft_lcd *myLcdDevice);
int	jpeg_to_binary					(image *imageBuf);
int	image_send_to_memory				(tft_lcd *myLcdDevice, image *write_image, uint8_t init_cmd);
int 	send_string					(tft_lcd *myLcdDevice, uint8_t *buf, int strLen, uint8_t pos_x, uint8_t pos_y, uint8_t color);	
void 	serial_send					(tft_lcd *myLcdDevice, uint8_t *buf, int bufSize);

void 	set_background_image				(tft_lcd *myLcdDevice, uint8_t address);
void 	set_parts_image					(tft_lcd *myLcdDevice, uint8_t address, uint8_t pos_x, uint8_t pos_y);
void	set_parts_clear					(tft_lcd *myLcdDevice);
void	set_string_clear				(tft_lcd *myLcdDevice);	// ADD -> 171026, string clear, winston
void	set_guide_clear				(tft_lcd *myLcdDevice);	



#endif /* LIB_TFT_LCD_TFT_LCD_CONTROL_H_ */
