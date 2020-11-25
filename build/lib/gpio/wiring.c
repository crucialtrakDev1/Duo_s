//
#include "wiring.h"

int digitalPinMode(int pin, int dir) {
	FILE * fd;
	char fName[128];
	char val[4];
	int ival = 0;

	if(dir == OUTPUT1) ival = 0;

	else if(dir == LOUTPUT) {
		ival = 0;
		dir = OUTPUT1;
	}
	else if(dir == HOUTPUT) {
		ival = 1;
		dir = OUTPUT1;
	}

	sprintf(fName, "/sys/class/gpio/gpio%d/direction", pin);
	if(access(fName, F_OK)) {
		if((fd = fopen("/sys/class/gpio/export", "w")) == NULL) {
			printf("Error: unable to export pin\n");
			return -1;
		}
		fprintf(fd, "%d\n", pin);
		fclose(fd);
	}

	if((fd = fopen(fName, "w+")) == NULL) {
		printf("Error: can't open pin direction\n");
		return -1;
	}
	fgets(val, 2, fd);
	fseek(fd, 0, SEEK_SET);

	if(dir == OUTPUT1) {
		if(val[0] == 'i') {
			if(ival)
				fprintf(fd, "high\n");
			else
				fprintf(fd, "low\n");
		}
	}
	else {
		if(val[0] == 'o') {
			fprintf(fd, "in\n");
		}
	}
	fclose(fd);

	return 0;
}

int digitalRead(int pin) {
	FILE * fd;
	char fName[128];
	char val[4];

	sprintf(fName, "/sys/class/gpio/gpio%d/value", pin);
	if((fd = fopen(fName, "r")) == NULL) {
		printf("Error: can't open pin value\n");
		return -1;
	}
	fgets(val, 2, fd);
	fclose(fd);

	return atoi(val);
}

int digitalWrite(int pin, int val) {
	FILE * fd;
	char fName[128];

	sprintf(fName, "/sys/class/gpio/gpio%d/value", pin);
	if((fd = fopen(fName, "w")) == NULL) {
		printf("Error: can't open pin value\n");
		return -1;
	}
	if(val == HIGH1) {
		fprintf(fd, "1\n");
	}
	else {
		fprintf(fd, "0\n");
	}
	fclose(fd);

	return 0;
}

