//*****************************************************************************
//
// switch_task.c - A simple switch task to process the buttons.
//
// Copyright (c) 2012-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.4.178 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "./../include/inc/hw_memmap.h"
#include "./../include/inc/hw_types.h"
#include "./../include/inc/hw_gpio.h"
#include "./../include/driverlib/sysctl.h"
#include "./../include/driverlib/gpio.h"
#include "./../include/driverlib/rom.h"
#include "./../include/drivers/buttons.h"
#include "./../include/utils/uartstdio.h"
#include "switch_task.h"
#include "led_task.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "BSP.h"

//*****************************************************************************
//
// The stack size for the display task.
//
//*****************************************************************************
#define SWITCHTASKSTACKSIZE        128         // Stack size in words
#define SW1_TEXT_MODE 	0
#define SW1_BUBBLE_MODE 1
#define SW2_UNLOCKED		0
#define SW2_LOCKED			1
#define MKII_SW1				0x30
#define MKII_SW2				0x40
extern uint8_t sw1_mode, sw2_mode;
extern uint16_t Red, Green, Blue;
extern uint16_t JoyX, JoyY;
extern uint16_t AccX, AccY, AccZ;
extern uint16_t prev_AccX, prev_AccY, prev_AccZ;
extern uint8_t current;

extern xQueueHandle g_pLEDQueue, g_pAccelerometerQueue;
extern xSemaphoreHandle g_pUARTSemaphore;
extern xSemaphoreHandle g_pAccelerometerSemaphore, g_pModeSemaphore;

//*****************************************************************************
//
// This task reads the buttons' state and passes this information to LEDTask.
//
//*****************************************************************************
static void
SwitchTask(void *pvParameters)
{
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;
    uint8_t ui8CurButtonState, ui8PrevButtonState;
    uint8_t ui8Message;
		uint8_t prev_button1 = sw1_mode, prev_button2 = sw2_mode;

    ui8CurButtonState = ui8PrevButtonState = 0;

    //
    // Get the current tick count.
    //
    ui16LastTime = xTaskGetTickCount();

    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Poll the debounced state of the buttons.
        //
        //ui8CurButtonState = ButtonsPoll(0, 0);
			
				//
				// Poll the MDII buttons
				//
				poll_MDKII(&prev_button1, &prev_button2);
			
				//
				// Pass the value of the button pressed to LEDTask.
				//
				if(xQueueSend(g_pLEDQueue, &ui8Message, portMAX_DELAY) != pdPASS)
				{
						//
						// Error. The queue should never be full. If so print the
						// error message on UART and wait for ever.
						//
						UARTprintf("\nQueue full. This should never happen.\n");
						while(1)
						{
						}
				}
				
				//
        // Wait for the required amount of time to check back.
        //
        vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);
    }
}

//*****************************************************************************
//
// Initializes the switch task.
//
//*****************************************************************************
uint32_t
SwitchTaskInit(void)
{
    //
    // Unlock the GPIO LOCK register for Right button to work.
    //
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0xFF;

    //
    // Initialize the buttons
    //
    //ButtonsInit();
	
		//
		// BSP Buttons Init
		//
		BSP_Joystick_Init(); // Valvano 
		BSP_Button1_Init(); // Valvano 
		BSP_Button2_Init(); // Valvano 
		
    //
    // Create the switch task.
    //
    if(xTaskCreate(SwitchTask, (const portCHAR *)"Switch",
                   SWITCHTASKSTACKSIZE, NULL, tskIDLE_PRIORITY +
                   PRIORITY_SWITCH_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}

void poll_MDKII(uint8_t *prev_button1, uint8_t *prev_button2){
	//BSP_Joystick_Input(&JoyX, &JoyY, &current);
	current = BSP_Button1_Input();	
	if(!current){
		sw1_mode = !sw1_mode;
		//
		// Guard UART from concurrent access.
		//
		if( sw1_mode == SW1_TEXT_MODE ){
			xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
			UARTprintf("Switch One is pressed.\n");
			UARTprintf("In TEXT mode.\n");
			xSemaphoreGive(g_pUARTSemaphore);
		}
		else{
			xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
			UARTprintf("Switch One is pressed.\n");
			UARTprintf("In BUBBLE mode.\n");
			xSemaphoreGive(g_pUARTSemaphore);
		}		
	}
	current = BSP_Button2_Input();
	if(!current){
		sw2_mode = !sw2_mode;
		//
		// Guard UART from concurrent access.
		//
		if( sw2_mode == SW2_LOCKED ){
			xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
			UARTprintf("Switch Two is pressed.\n");
			UARTprintf("In LOCK mode.\n");
			xSemaphoreGive(g_pUARTSemaphore);
		}
		else if( sw2_mode == SW2_UNLOCKED ){
			xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
			UARTprintf("Switch Two is pressed.\n");
			UARTprintf("In UNLOCK mode.\n");
			xSemaphoreGive(g_pUARTSemaphore);
		}
		
	}
}
