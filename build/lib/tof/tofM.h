/*
 * tofM.h
 *
 *  Created on: Aug 29, 2017
 *      Author: root
 */

#ifndef LIB_TOF_TOFM_H_
#define LIB_TOF_TOFM_H_





#define TOF_OPEN_SUCCESS	1
#define TOF_OPEN_FAIL		0


#include "vl53l0x_platform.h"
#include "vl53l0x_api.h"


//VL53L0X API function
void 			print_pal_error				(VL53L0X_Error Status);
void 			print_range_status			(VL53L0X_RangingMeasurementData_t* pRangingMeasurementData);
VL53L0X_Error 	WaitMeasurementDataReady	(VL53L0X_DEV Dev);
VL53L0X_Error 	WaitStopCompleted			(VL53L0X_DEV Dev);
uint16_t 		rangingTest					(int no_of_measurements, VL53L0X_Dev_t *pMyDevice);


//BACS DUO to use function
int 			tof_open					(VL53L0X_Dev_t *pMyDevice);
uint16_t 		tof_get_data				(int no_of_measurements, VL53L0X_Dev_t *pMyDevice);
void 			tof_close					(VL53L0X_Dev_t *pMyDevice);

int 			tof_init_cal_data			(VL53L0X_Dev_t *pMyDevice);
uint16_t		tof_get_cal_data			(int no_of_measurements, VL53L0X_Dev_t *pMyDevice);
void 			tof_close_cal_data			(VL53L0X_Dev_t *pMyDevice);


//event handle function
void 			sigint_handler				(int signo, VL53L0X_Dev_t *pMyDevice);





#endif /* LIB_TOF_TOFM_H_ */
