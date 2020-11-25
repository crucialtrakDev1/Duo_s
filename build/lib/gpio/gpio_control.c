/*
 * gpio_control.c
 *
 *  Created on: Aug 10, 2017
 *      Author: root
 */


#include "gpio_control.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void gpio_open_port(int pin_number)
{
	system("sudo chmod 777 -R /sys/class/gpio/*");

	switch(pin_number)
	{

		case 10 :	//PIN10 --> GPIO462
		{
			system("echo 462 > /sys/class/gpio/export");
			printf("gpio open [%d] \n ", pin_number);
		}break;


		case 11 :	//PIN11 --> GPIO442
		{
			system("echo 442 > /sys/class/gpio/export");
			printf("gpio open [%d] \n ", pin_number);
		}break;


		case 12 :	//PIN12 --> GPIO432
		{
			system("echo 432 > /sys/class/gpio/export");
			printf("gpio open [%d] \n ", pin_number);
		}break;

		case 15 :	//PIN15 --> GPIO425
		{
			system("echo 425 > /sys/class/gpio/export");
			printf("gpio open [%d] \n ", pin_number);
		}break;

		case 16 :	//PIN16 --> GPIO424
		{
			system("echo 424 > /sys/class/gpio/export");
			printf("gpio open [%d] \n ", pin_number);
		}break;



		default :
		{
			printf("err : this port can't use [%d] \n", pin_number);
		}break;

	}
}


void gpio_init_port(int pin_number, int pin_set_value)
{
	system("sudo chmod 777 -R /sys/class/gpio/*");
	switch(pin_number)
	{

		case 10 :	//PIN10 --> GPIO462
		{
			if(pin_set_value == 0)
			{
				system("echo out > /sys/class/gpio/gpio462/direction");
				printf("gpio output set [%d] \n ", pin_number);
			}
			else if(pin_set_value == 1)
			{
				system("echo in > /sys/class/gpio/gpio462/direction");
				printf("gpio input set [%d] \n ", pin_number);
			}
			else
			{
				printf("wrong set gpio[%d] set[%d]  \n ", pin_number, pin_set_value);
			}
		}break;


		case 11 :	//PIN11 --> GPIO442
		{
			if(pin_set_value == 0)
			{
				system("echo out > /sys/class/gpio/gpio442/direction");
				printf("gpio output set [%d] \n ", pin_number);
			}
			else if(pin_set_value == 1)
			{
				system("echo in > /sys/class/gpio/gpio442/direction");
				printf("gpio input set [%d] \n ", pin_number);
			}
			else
			{
				printf("wrong set gpio[%d] set[%d]  \n ", pin_number, pin_set_value);
			}
		}break;

		case 12 :	//PIN12 --> GPIO432
		{
			if(pin_set_value == 0)
			{
				system("echo out > /sys/class/gpio/gpio432/direction");
				printf("gpio output set [%d] \n ", pin_number);
			}
			else if(pin_set_value == 1)
			{
				system("echo in > /sys/class/gpio/gpio432/direction");
				printf("gpio input set [%d] \n ", pin_number);
			}
			else
			{
				printf("wrong set gpio[%d] set[%d]  \n ", pin_number, pin_set_value);
			}
		}break;

		case 15 :	//PIN15 --> GPIO425
		{
			if(pin_set_value == 0)
			{
				system("echo out > /sys/class/gpio/gpio425/direction");
				printf("gpio output set [%d] \n ", pin_number);
			}
			else if(pin_set_value == 1)
			{
				system("echo in > /sys/class/gpio/gpio425/direction");
				printf("gpio input set [%d] \n ", pin_number);
			}
			else
			{
				printf("wrong set gpio[%d] set[%d]  \n ", pin_number, pin_set_value);
			}
		}break;

		case 16 :	//PIN16 --> GPIO424
		{
			if(pin_set_value == 0)
			{
				system("echo out > /sys/class/gpio/gpio424/direction");
				printf("gpio output set [%d] \n ", pin_number);
			}
			else if(pin_set_value == 1)
			{
				system("echo in > /sys/class/gpio/gpio424/direction");
				printf("gpio input set [%d] \n ", pin_number);
			}
			else
			{
				printf("wrong set gpio[%d] set[%d]  \n ", pin_number, pin_set_value);
			}
		}break;


		default :
		{
				//not to do
		}break;

	}

}


void gpio_set_value(int pin_number, int set_value)
{
	system("sudo chmod 777 -R /sys/class/gpio/*");
	switch(pin_number)
	{

		case 10 :	//PIN10 --> GPIO462
		{
			if(set_value == 0)
			{
				system("echo 0 > /sys/class/gpio/gpio462/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
			else
			{
				system("echo 1 > /sys/class/gpio/gpio462/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}

		}break;


		case 11 :	//PIN11 --> GPIO442
		{
			if(set_value == 0)
			{
				system("echo 0 > /sys/class/gpio/gpio442/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
			else
			{
				system("echo 1 > /sys/class/gpio/gpio442/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
		}break;

		case 12 :	//PIN12 --> GPIO432
		{
			if(set_value == 0)
			{
				system("echo 0 > /sys/class/gpio/gpio432/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
			else
			{
				system("echo 1 > /sys/class/gpio/gpio432/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
		}break;

		case 15 :	//PIN15 --> GPIO425
		{
			if(set_value == 0)
			{
				system("echo 1 > /sys/class/gpio/gpio425/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
			else
			{
				system("echo 0 > /sys/class/gpio/gpio425/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
		}break;

		case 16 :	//PIN16 --> GPIO424
		{
			if(set_value == 0)
			{
				system("echo 1 > /sys/class/gpio/gpio424/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
			else
			{
				system("echo 0 > /sys/class/gpio/gpio424/value");
				printf("gpio [%d] latch -> %d \n ", pin_number, set_value);
			}
		}break;



		default :
		{
				//not to do
		}break;
	}
}
