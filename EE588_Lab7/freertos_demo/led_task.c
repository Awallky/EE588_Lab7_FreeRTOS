//*****************************************************************************
//
// led_task.c - A simple flashing LED task.
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

#define SW1_TEXT_MODE 	0
#define SW1_BUBBLE_MODE 1
#define SW2_UNLOCKED		0
#define SW2_LOCKED			1
#define ACCEL_READ			0x08
#define MKII_SW1				0x30
#define MKII_SW2				0x40

extern uint8_t sw1_mode, sw2_mode;
extern uint16_t Red, Green, Blue;
extern uint16_t JoyX, JoyY;
extern uint16_t AccX, AccY, AccZ;
extern uint16_t prev_AccX, prev_AccY, prev_AccZ;
extern uint8_t current;
extern xSemaphoreHandle g_pAccelerometerSemaphore, g_pModeSemaphore;

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

//*****************************************************************************
//
// The stack size for the LED toggle task.
//
//*****************************************************************************
#define LEDTASKSTACKSIZE        128         // Stack size in words

//*****************************************************************************
//
// The item size and queue size for the LED message queue.
//
//*****************************************************************************
#define LED_ITEM_SIZE           sizeof(uint8_t)
#define LED_QUEUE_SIZE          5

//*****************************************************************************
//
// Default LED toggle delay value. LED toggling frequency is twice this number.
//
//*****************************************************************************
#define LED_TOGGLE_DELAY        250

//*****************************************************************************
//
// The queue that holds messages sent to the LED task.
//
//*****************************************************************************
xQueueHandle g_pLEDQueue, g_pAccelerometerQueue;

//
// [G, R, B] range is 0 to 0xFFFF per color.
//
static uint32_t g_pui32Colors[3] = { 0x0000, 0x0000, 0x0000 };
static uint8_t g_ui8ColorsIndx;

extern xSemaphoreHandle g_pUARTSemaphore, g_pSW1Semaphore, g_pSW2Semaphore;

//*****************************************************************************
//
// This task toggles the user selected LED at a user selected frequency. User
// can make the selections by pressing the left and right buttons.
//
//*****************************************************************************
static void
LEDTask(void *pvParameters)
{
    portTickType ui32WakeTime;
    uint32_t ui32LEDToggleDelay;
    uint8_t i8Message;
		uint8_t count = 0;
		

    //
    // Initialize the LED Toggle Delay to default value.
    //
    ui32LEDToggleDelay = LED_TOGGLE_DELAY;

    //
    // Get the current tick count.
    //
    ui32WakeTime = xTaskGetTickCount();

	
    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Read the next message, if available on queue.
        //
        if(xQueueReceive(g_pLEDQueue, &i8Message, 0) == pdPASS)
        {
						count = count + 1;
						xSemaphoreTake(g_pAccelerometerSemaphore, portMAX_DELAY);
						if(count == 5){
							count = 0;
							DisableInterrupts();
							if(sw1_mode == SW1_TEXT_MODE){
								if(sw2_mode == SW2_UNLOCKED){
									// print accelerometer status			
									BSP_LCD_DrawString(0, 5, "AccX=    ", BSP_LCD_Color565(255, 255, 255));
									BSP_LCD_SetCursor(5, 5);
									BSP_LCD_OutUDec((uint32_t)AccX, BSP_LCD_Color565(255, 0, 255));
									BSP_LCD_DrawString(0, 6, "AccY=    ", BSP_LCD_Color565(255, 255, 255));
									BSP_LCD_SetCursor(5, 6);
									BSP_LCD_OutUDec((uint32_t)AccY, BSP_LCD_Color565(255, 0, 255));
									BSP_LCD_DrawString(0, 7, "AccZ=    ", BSP_LCD_Color565(255, 255, 255));
									BSP_LCD_SetCursor(5, 7);
									BSP_LCD_OutUDec((uint32_t)AccZ, BSP_LCD_Color565(255, 0, 255));
									BSP_LCD_DrawString(0, 8, "We really out here!", BSP_LCD_Color565(255, 255, 255));
									BSP_LCD_SetCursor(5, 8);
								}
								else if(sw2_mode == SW2_LOCKED){
									// print accelerometer status									
									BSP_LCD_DrawString(0, 5, "AccX=    ", BSP_LCD_Color565(255, 255, 255));
									BSP_LCD_SetCursor(5, 5);
									BSP_LCD_OutUDec((uint32_t)(prev_AccX), BSP_LCD_Color565(255, 0, 255));
									BSP_LCD_DrawString(0, 6, "AccY=    ", BSP_LCD_Color565(255, 255, 255));
									BSP_LCD_SetCursor(5, 6);
									BSP_LCD_OutUDec((uint32_t)(prev_AccY), BSP_LCD_Color565(255, 0, 255));
									BSP_LCD_DrawString(0, 7, "AccZ=    ", BSP_LCD_Color565(255, 255, 255));
									BSP_LCD_SetCursor(5, 7);
									BSP_LCD_OutUDec((uint32_t)(prev_AccZ), BSP_LCD_Color565(255, 0, 255));
								}
							}
							else if(sw1_mode == SW1_BUBBLE_MODE){
								if(sw2_mode == SW2_UNLOCKED){
									UARTprintf("Ummmm....wut?!\n");
								}
								else if(sw2_mode == SW2_LOCKED){
									UARTprintf("Ummmm....wut?!\n");
								}
							}
							EnableInterrupts();
						}
						xSemaphoreGive(g_pAccelerometerSemaphore);
        } // if(xQueueReceive(g_pLEDQueue, &i8Message, 0) == pdPASS)

        //
        // Wait for the required amount of time.
        //
        vTaskDelayUntil(&ui32WakeTime, ui32LEDToggleDelay / portTICK_RATE_MS);

        //
        // Wait for the required amount of time.
        //
        vTaskDelayUntil(&ui32WakeTime, ui32LEDToggleDelay / portTICK_RATE_MS);
			} // while(1)
}

//*****************************************************************************
//
// Initializes the LED task.
//
//*****************************************************************************
uint32_t
LEDTaskInit(void)
{
    //
    // Initialize the GPIOs and Timers that drive the three LEDs.
    //
    //RGBInit(1);
    //RGBIntensitySet(0.3f);

    //
    // Turn on the Green LED
    //
    //g_ui8ColorsIndx = 0;
    //g_pui32Colors[g_ui8ColorsIndx] = 0x8000;
    //RGBColorSet(g_pui32Colors);
	
		//
		// BSP Pins Init
		//
		BSP_LCD_Init();
		BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));	
	
    //
    // Print the current loggling LED and frequency.
    //
    UARTprintf("\nLed %d is blinking. [R, G, B]\n", g_ui8ColorsIndx);
    UARTprintf("Led blinking frequency is %d ms.\n", (LED_TOGGLE_DELAY * 2));

    //
    // Create a queue for sending messages to the LED task.
    //
    g_pLEDQueue = xQueueCreate(LED_QUEUE_SIZE, LED_ITEM_SIZE);

    //
    // Create the LED task.
    //
    if(xTaskCreate(LEDTask, (const portCHAR *)"LED", LEDTASKSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_LED_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}
