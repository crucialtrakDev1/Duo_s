#ifndef _WIRING_H_
#define _WIRING_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define LOW0 	0
#define HIGH1 	1
#define INPUT0 	0
#define OUTPUT1	1
#define LOUTPUT 2
#define HOUTPUT	3

int digitalPinMode(int pin, int dir);
int digitalRead(int pin);
int digitalWrite(int pin, int val);

#endif
