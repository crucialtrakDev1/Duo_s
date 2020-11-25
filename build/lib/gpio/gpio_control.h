/*
 * gpio_control.h
 *
 *  Created on: Aug 10, 2017
 *      Author: root
 */


#ifndef GPIO_CONTROL_H_
#define GPIO_CONTROL_H_


void gpio_open_port(int pin_number);
void gpio_init_port(int pin_number, int pin_set_value);
void gpio_set_value(int pin_number, int set_value);


#endif /* GPIO_CONTROL_H_ */


