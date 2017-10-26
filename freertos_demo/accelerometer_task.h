#ifndef ACCELEROMETER_TASK_H
#define ACCELEROMETER_TASK_H

#include <stdbool.h>
#include <stdint.h>
#include "./../ti/TivaWare_C_Series-2.1.4.178/inc/hw_memmap.h"
#include "./../ti/TivaWare_C_Series-2.1.4.178/inc/hw_types.h"
#include "./../ti/TivaWare_C_Series-2.1.4.178/driverlib/gpio.h"
#include "./../ti/TivaWare_C_Series-2.1.4.178/driverlib/rom.h"
#include "./../ti/TivaWare_C_Series-2.1.4.178/examples/boards/ek-tm4c123gxl/drivers/rgb.h"
#include "./../ti/TivaWare_C_Series-2.1.4.178/examples/boards/ek-tm4c123gxl/drivers/buttons.h"
#include "./../ti/TivaWare_C_Series-2.1.4.178/utils/uartstdio.h"
#include "led_task.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "./../inc/tm4c123gh6pm.h"
#include "BSP.h"

#define UNLOCKED 0
#define LOCKED 1
#define ACCELEROMETERTASKSIZE       128         // Stack size in words

uint32_t Accelerometer_Init(void);
static void monitor_accelerometers(void);
#endif
