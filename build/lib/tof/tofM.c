/*
 * tofM.c
 *
 *  Created on: Aug 29, 2017
 *      Author: root
 */
#include "tofM.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "required_version.h"



void print_pal_error(VL53L0X_Error Status){
    char buf[VL53L0X_MAX_STRING_LENGTH];
    VL53L0X_GetPalErrorString(Status, buf);
    printf("API Status: %i : %s\n", Status, buf);
}

void print_range_status(VL53L0X_RangingMeasurementData_t* pRangingMeasurementData){
    char buf[VL53L0X_MAX_STRING_LENGTH];
    uint8_t RangeStatus;

    /*
     * New Range Status: data is valid when pRangingMeasurementData->RangeStatus = 0
     */

    RangeStatus = pRangingMeasurementData->RangeStatus;

    VL53L0X_GetRangeStatusString(RangeStatus, buf);
    printf("Range Status: %i : %s\n", RangeStatus, buf);

}

VL53L0X_Error WaitMeasurementDataReady(VL53L0X_DEV Dev) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t NewDatReady=0;
    uint32_t LoopNb;

    // Wait until it finished
    // use timeout to avoid deadlock
    if (Status == VL53L0X_ERROR_NONE) {
        LoopNb = 0;
        do {
            Status = VL53L0X_GetMeasurementDataReady(Dev, &NewDatReady);
            if ((NewDatReady == 0x01) || Status != VL53L0X_ERROR_NONE) {
                break;
            }
            LoopNb = LoopNb + 1;
            VL53L0X_PollingDelay(Dev);
        } while (LoopNb < VL53L0X_DEFAULT_MAX_LOOP);

        if (LoopNb >= VL53L0X_DEFAULT_MAX_LOOP) {
            Status = VL53L0X_ERROR_TIME_OUT;
        }
    }

    return Status;
}

VL53L0X_Error WaitStopCompleted(VL53L0X_DEV Dev) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint32_t StopCompleted=0;
    uint32_t LoopNb;

    // Wait until it finished
    // use timeout to avoid deadlock
    if (Status == VL53L0X_ERROR_NONE) {
        LoopNb = 0;
        do {
            Status = VL53L0X_GetStopCompletedStatus(Dev, &StopCompleted);
            if ((StopCompleted == 0x00) || Status != VL53L0X_ERROR_NONE) {
                break;
            }
            LoopNb = LoopNb + 1;
            VL53L0X_PollingDelay(Dev);
        } while (LoopNb < VL53L0X_DEFAULT_MAX_LOOP);

        if (LoopNb >= VL53L0X_DEFAULT_MAX_LOOP) {
            Status = VL53L0X_ERROR_TIME_OUT;
        }

    }

    return Status;
}

uint16_t rangingTest(int no_of_measurements, VL53L0X_Dev_t *pMyDevice)
{
    VL53L0X_RangingMeasurementData_t    RangingMeasurementData;
    VL53L0X_RangingMeasurementData_t   *pRangingMeasurementData    = &RangingMeasurementData;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    uint8_t VhvSettings;
    uint8_t PhaseCal;

    uint16_t Millimeter_ret_value;



    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_StaticInit\n");
        Status = VL53L0X_StaticInit(pMyDevice); // Device Initialization
        // StaticInit will set interrupt by default
        print_pal_error(Status);
    } 
    
    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_PerformRefCalibration\n");
        Status = VL53L0X_PerformRefCalibration(pMyDevice,
        		&VhvSettings, &PhaseCal); // Device Initialization
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_PerformRefSpadManagement\n");
        Status = VL53L0X_PerformRefSpadManagement(pMyDevice,
        		&refSpadCount, &isApertureSpads); // Device Initialization
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {

        printf ("Call of VL53L0X_SetDeviceMode\n");
        Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING); // Setup in single ranging mode
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
		printf ("Call of VL53L0X_StartMeasurement\n");
		Status = VL53L0X_StartMeasurement(pMyDevice);
		print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        uint32_t measurement;

//        uint16_t* pResults = (uint16_t*)malloc(sizeof(uint16_t) * no_of_measurements);

        for(measurement=0; measurement<no_of_measurements; measurement++)
        {

            Status = WaitMeasurementDataReady(pMyDevice);

            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_GetRangingMeasurementData(pMyDevice, pRangingMeasurementData);

//                *(pResults + measurement) = pRangingMeasurementData->RangeMilliMeter;
                Millimeter_ret_value = pRangingMeasurementData->RangeMilliMeter;
//                //printf("In loop measurement %d: %d\n", measurement, pRangingMeasurementData->RangeMilliMeter);

                // Clear the interrupt
                VL53L0X_ClearInterruptMask(pMyDevice, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
                // VL53L0X_PollingDelay(pMyDevice);
            }
            else
            {
                break;
            }
        }

//        if(Status == VL53L0X_ERROR_NONE)
//        {
//            for(measurement=0; measurement<no_of_measurements; measurement++)
//            {
//                printf("measurement %d: %d\n", measurement, *(pResults + measurement));
//            }
//        }
//
//        free(pResults);
    }


    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_StopMeasurement\n");
        Status = VL53L0X_StopMeasurement(pMyDevice);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Wait Stop to be competed\n");
        Status = WaitStopCompleted(pMyDevice);
    }

    if(Status == VL53L0X_ERROR_NONE)
	Status = VL53L0X_ClearInterruptMask(pMyDevice,
		VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);


	if(Status != VL53L0X_ERROR_NONE)
	{
		exit(1);
	}
	else
	    return Millimeter_ret_value;
	
}


int	tof_open(VL53L0X_Dev_t *pMyDevice)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    VL53L0X_Version_t                   Version;
    VL53L0X_Version_t                  *pVersion   = &Version;
    VL53L0X_DeviceInfo_t                DeviceInfo;

    int32_t status_int;

     printf ("VL53L0X PAL Continuous Ranging example\n\n");

     // Initialize Comms
     pMyDevice->I2cDevAddr      = TOF_I2C_ADDR;

     printf("Open Device : %s \n\r", TOF_I2C_BUS);
     pMyDevice->fd = VL53L0X_i2c_init(TOF_I2C_BUS, pMyDevice->I2cDevAddr);
     if (pMyDevice->fd < 0) {
         Status = VL53L0X_ERROR_CONTROL_INTERFACE;
         printf ("Failed to init\n");
         close(pMyDevice->fd);
	return -1;
     }

     /*
      *  Get the version of the VL53L0X API running in the firmware
      */

     if(Status == VL53L0X_ERROR_NONE)
     {
         status_int = VL53L0X_GetVersion(pVersion);
         if (status_int != 0)
             Status = VL53L0X_ERROR_CONTROL_INTERFACE;
     }

     /*
      *  Verify the version of the VL53L0X API running in the firmrware
      */

     if(Status == VL53L0X_ERROR_NONE)
     {
         if( pVersion->major != VERSION_REQUIRED_MAJOR ||
             pVersion->minor != VERSION_REQUIRED_MINOR ||
             pVersion->build != VERSION_REQUIRED_BUILD )
         {
             printf("VL53L0X API Version Error: Your firmware has %d.%d.%d (revision %d). This example requires %d.%d.%d.\n",
                 pVersion->major, pVersion->minor, pVersion->build, pVersion->revision,
                 VERSION_REQUIRED_MAJOR, VERSION_REQUIRED_MINOR, VERSION_REQUIRED_BUILD);
         }
     }

     // End of implementation specific
     if(Status == VL53L0X_ERROR_NONE)
     {
         printf ("Call of VL53L0X_DataInit\n");
         Status = VL53L0X_DataInit(pMyDevice); // Data initialization
         print_pal_error(Status);
     }

     if(Status == VL53L0X_ERROR_NONE)
     {
         Status = VL53L0X_GetDeviceInfo(pMyDevice, &DeviceInfo);
     }
     if(Status == VL53L0X_ERROR_NONE)
     {
         printf("VL53L0X_GetDeviceInfo:\n");
         printf("Device Name : %s\n", DeviceInfo.Name);
         printf("Device Type : %s\n", DeviceInfo.Type);
         printf("Device ID : %s\n", DeviceInfo.ProductId);
         printf("ProductRevisionMajor : %d\n", DeviceInfo.ProductRevisionMajor);
         printf("ProductRevisionMinor : %d\n", DeviceInfo.ProductRevisionMinor);

         if ((DeviceInfo.ProductRevisionMinor != 1) && (DeviceInfo.ProductRevisionMinor != 1))
         {
         	printf("Error expected cut 1.1 but found cut %d.%d\n",
         			DeviceInfo.ProductRevisionMajor, DeviceInfo.ProductRevisionMinor);
         	Status = VL53L0X_ERROR_NOT_SUPPORTED;
         }

     }
     if(Status == VL53L0X_ERROR_NONE)
    	 return 1;
     else
    	 return 0;

}


uint16_t tof_get_data(int no_of_measurements, VL53L0X_Dev_t *pMyDevice)
{
	return rangingTest(no_of_measurements, pMyDevice);
}


void tof_close(VL53L0X_Dev_t *pMyDevice)
{
    close(pMyDevice->fd);
    printf ("come %s\n", __func__);
    VL53L0X_i2c_close();
}


int 			tof_init_cal_data			(VL53L0X_Dev_t *pMyDevice)
{
	
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    uint8_t VhvSettings;
    uint8_t PhaseCal;
    
    
    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_StaticInit\n");
        Status = VL53L0X_StaticInit(pMyDevice); // Device Initialization
        // StaticInit will set interrupt by default
        print_pal_error(Status);
    } 
    
    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_PerformRefCalibration\n");
        Status = VL53L0X_PerformRefCalibration(pMyDevice,
        		&VhvSettings, &PhaseCal); // Device Initialization
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_PerformRefSpadManagement\n");
        Status = VL53L0X_PerformRefSpadManagement(pMyDevice,
        		&refSpadCount, &isApertureSpads); // Device Initialization
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {

        printf ("Call of VL53L0X_SetDeviceMode\n");
        // Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING); // Setup in single ranging mode
        Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING); // Setup in single ranging mode
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
		printf ("Call of VL53L0X_StartMeasurement\n");
		Status = VL53L0X_StartMeasurement(pMyDevice);
		print_pal_error(Status);
    }
    
    return Status; 
	
}

//FIXME
int myCompare (const void *first, const void *second)
{
	if (*(uint16_t*)first > *(uint16_t*)second)
		return 1;
	else if (*(uint16_t*)first < *(uint16_t*)second)
		return -1;
	else return 0;
}

uint16_t 			tof_get_cal_data			(int no_of_measurements, VL53L0X_Dev_t *pMyDevice)
{
//    uint16_t Millimeter_ret_value;
    uint16_t medianValue;
	
    VL53L0X_RangingMeasurementData_t    RangingMeasurementData;
    VL53L0X_RangingMeasurementData_t   *pRangingMeasurementData    = &RangingMeasurementData;
    
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	
    if(Status == VL53L0X_ERROR_NONE)
    {
        uint32_t measurement;
        uint16_t* pResults = (uint16_t*)malloc(sizeof(uint16_t) * no_of_measurements);

        for(measurement=0; measurement<no_of_measurements; measurement++)
        {
            Status = WaitMeasurementDataReady(pMyDevice);

            if(Status == VL53L0X_ERROR_NONE)
            {
                Status = VL53L0X_GetRangingMeasurementData(pMyDevice, pRangingMeasurementData);

                *(pResults + measurement) = pRangingMeasurementData->RangeMilliMeter;
//                Millimeter_ret_value = pRangingMeasurementData->RangeMilliMeter;
//                printf("%d, ", pResults[measurement]);

                // Clear the interrupt
                VL53L0X_ClearInterruptMask(pMyDevice, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
                // VL53L0X_PollingDelay(pMyDevice);
            }
            else
            {
                break;
            }
        }
//        printf("\n\r");
		qsort(pResults, no_of_measurements, sizeof(uint16_t), myCompare);
		medianValue = pResults[no_of_measurements/2];
        
        return medianValue;
    }	
    else
    {
        exit(1);
    }
    
}


void 			tof_close_cal_data			(VL53L0X_Dev_t *pMyDevice)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	
    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Call of VL53L0X_StopMeasurement\n");
        Status = VL53L0X_StopMeasurement(pMyDevice);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        printf ("Wait Stop to be competed\n");
        Status = WaitStopCompleted(pMyDevice);
    }

    if(Status == VL53L0X_ERROR_NONE)
	Status = VL53L0X_ClearInterruptMask(pMyDevice, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
	
}
