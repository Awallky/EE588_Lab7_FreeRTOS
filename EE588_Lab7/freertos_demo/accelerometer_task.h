// accelerometer_task.h
#ifndef ACCELEROMETER_TASK_H
#define ACCELEROMETER_TASK_H

#include <stdbool.h>
#include <stdint.h>
#include "./../include/inc/hw_memmap.h"
#include "./../include/inc/hw_types.h"
#include "./../include/driverlib/gpio.h"
#include "./../include/driverlib/rom.h"
#include "./../include/drivers/rgb.h"
#include "./../include/drivers/buttons.h"
#include "./../include/utils/uartstdio.h"
#include "led_task.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "BSP.h"

uint32_t AccelerometerTaskInit(void);
static void AccelerometerTask(void *pvParameters);

//*****************************************************************************
//
// The stack size for the Accelerometer Poll task.
//
//*****************************************************************************
#ifndef ACCELEROMETERTASKSTACKSIZE
	#define ACCELEROMETERTASKSTACKSIZE        128         // Stack size in words
#endif

//*****************************************************************************
//
// The item size and queue size for the LED message queue.
//
//*****************************************************************************
#define ACCELEROMETER_ITEM_SIZE           sizeof(uint8_t)
#define ACCELEROMETER_QUEUE_SIZE          5

//*****************************************************************************
//
// Default LED toggle delay value. LED toggling frequency is twice this number.
//
//*****************************************************************************
#define ACCELEROMETER_TOGGLE_DELAY        250

//
// [G, R, B] range is 0 to 0xFFFF per color.
//
//static uint32_t g_pui32Colors[3] = { 0x0000, 0x0000, 0x0000 };
//static uint8_t g_ui8ColorsIndx;
#endif
