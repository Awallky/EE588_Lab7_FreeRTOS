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
void checkbuttons(void);
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
xQueueHandle g_pLEDQueue;

//
// [G, R, B] range is 0 to 0xFFFF per color.
//
static uint32_t g_pui32Colors[3] = { 0x0000, 0x0000, 0x0000 };
static uint8_t g_ui8ColorsIndx;

extern xSemaphoreHandle g_pUARTSemaphore;
extern uint16_t Red, Green, Blue;
extern uint16_t JoyX, JoyY;
extern uint16_t AccX, AccY, AccZ;


void checkbuttons(void){
static uint8_t prev1 = 0, prev2 = 0, prevS = 0; // previous values
static uint8_t mode = 0;
uint8_t current;

BSP_Buzzer_Set(0);
BSP_Joystick_Input(&JoyX, &JoyY, &current);
BSP_Accelerometer_Input(&AccX, &AccY, &AccZ);
if((current == 0) && (prevS != 0)){
	// Select was pressed since last loop
	mode = (mode + 1)&0x03;
	Red = Green = Blue = 0;
	BSP_Buzzer_Set(512);           // beep until next interrupt (0.1 sec beep)
}
prevS = current;
if(mode == 0){
	// button mode
	current = BSP_Button1_Input();
	if((current == 0) && (prev1 != 0)){
		// Button1 was pressed since last loop
		Green = (Green + 64)&0x3FF;
	}
	prev1 = current;
	current = BSP_Button2_Input();
	if((current == 0) && (prev2 != 0)){
		// Button2 was pressed since last loop
		Blue = (Blue + 64)&0x3FF;
	}
	prev2 = current;
	Red = (Red + 1)&0x3FF;
} else if(mode == 1){
	// joystick mode
	Green = JoyX;
	Blue = JoyY;
	Red = 0;
} else if (mode == 2){
	// accelerometer mode
	if((AccX < 325) && (AccY > 325) && (AccY < 675) && (AccZ > 325) && (AccZ < 675)){
		Red = 500; Green = 0; Blue = 0;
	} else if((AccY < 325) && (AccX > 325) && (AccX < 675) && (AccZ > 325) && (AccZ < 675)){
		Red = 350; Green = 350; Blue = 0;
	} else if((AccX > 675) && (AccY > 325) && (AccY < 675) && (AccZ > 325) && (AccZ < 675)){
		Red = 0; Green = 500; Blue = 0;
	} else if((AccY > 675) && (AccX > 325) && (AccX < 675) && (AccZ > 325) && (AccZ < 675)){
		Red = 0; Green = 0; Blue = 500;
	}
} else{
}
BSP_RGB_Set(Red, Green, Blue);
}

// return the number of digits
int numlength(uint32_t n){
  if(n < 10) return 1;
  if(n < 100) return 2;
  if(n < 1000) return 3;
  if(n < 10000) return 4;
  if(n < 100000) return 5;
  if(n < 1000000) return 6;
  if(n < 10000000) return 7;
  if(n < 100000000) return 8;
  if(n < 1000000000) return 9;
  return 10;
}

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
	
	int count = 0;
	uint32_t i, time;
	int16_t color;
	
	//
	// Initialize the LED Toggle Delay to default value.
	//
	ui32LEDToggleDelay = LED_TOGGLE_DELAY;

	//
	// Get the current tick count.
	//
	ui32WakeTime = xTaskGetTickCount();
	
	while(1){
		WaitForInterrupt();
		count = count + 1;
		if(count == 10){
			count = 0;
			// print LED status
			BSP_LCD_DrawString(0, 0, "Red=    ", BSP_LCD_Color565(255, 0, 0));
			BSP_LCD_SetCursor(4, 0);
			BSP_LCD_OutUDec((uint32_t)Red, BSP_LCD_Color565(255, 0, 0));
			BSP_LCD_DrawString(0, 1, "Green=    ", BSP_LCD_Color565(0, 255, 0));
			BSP_LCD_SetCursor(6, 1);
			BSP_LCD_OutUDec((uint32_t)Green, BSP_LCD_Color565(0, 255, 0));
			BSP_LCD_DrawString(0, 2, "Blue=    ", BSP_LCD_Color565(0, 0, 255));
			BSP_LCD_SetCursor(5, 2);
			BSP_LCD_OutUDec((uint32_t)Blue, BSP_LCD_Color565(0, 0, 255));
			// print joystick status
			BSP_LCD_DrawString(0, 3, "JoyX=    ", BSP_LCD_Color565(255, 255, 255));
			BSP_LCD_SetCursor(5, 3);
			BSP_LCD_OutUDec((uint32_t)JoyX, BSP_LCD_Color565(255, 0, 255));
			BSP_LCD_DrawString(0, 4, "JoyY=    ", BSP_LCD_Color565(255, 255, 255));
			BSP_LCD_SetCursor(5, 4);
			BSP_LCD_OutUDec((uint32_t)JoyY, BSP_LCD_Color565(255, 0, 255));
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
		
			// print the time
			time = BSP_Time_Get(); // in usec
			color = LCD_LIGHTGREEN;
			BSP_LCD_DrawString(0, 11, "Time=  :  .      ", BSP_LCD_Color565(255, 255, 255));
			BSP_LCD_SetCursor(7-numlength(time/60000000), 11);
			BSP_LCD_OutUDec(time/60000000, color);
			BSP_LCD_SetCursor(8, 11);
			if(numlength((time%60000000)/1000000) == 1){
				// print a leading zero, which BSP_LCD_OutUDec() does not do automatically
				// so 1/60 prints as ":01" instead of ":1 "
				BSP_LCD_OutUDec(0, color);
			}
			BSP_LCD_OutUDec((time%60000000)/1000000, color);
			BSP_LCD_SetCursor(11, 11);
			for(i=numlength(time%1000000); i<6; i=i+1){
				// print any leading zeroes, which BSP_LCD_OutUDec() does not do automatically
				// so 1/1,000,000 prints as ":000001" instead of ".1 "
				BSP_LCD_OutUDec(0, color);
			}
			BSP_LCD_OutUDec(time%1000000, color);
		}
	}
	
}


//*****************************************************************************
//
// Initializes the LED task.
//
//*****************************************************************************
uint32_t
LEDTaskInit(void)
{	
	BSP_RGB_Init(0, 0, 0);
	BSP_Buzzer_Init(0);	
	BSP_PeriodicTask_Init(&checkbuttons, 10, 2);
	BSP_Time_Init();
	BSP_LCD_Init();
	BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));
	
	//
	// Initialize the GPIOs and Timers that drive the three LEDs.
	//
	RGBInit(1);
	RGBIntensitySet(0.3f);
	
	//
	// Turn on the Green LED
	//
	g_ui8ColorsIndx = 0;
	g_pui32Colors[g_ui8ColorsIndx] = 0x8000;
	RGBColorSet(g_pui32Colors);
	
	//
	// Print the current loggling LED and frequency.
	//
	UARTprintf("\nLed %d is blinking. [R, G, B]\n", g_ui8ColorsIndx);
	URTprintf("Led blinking frequency is %d ms.\n", (LED_TOGGLE_DELAY * 2));
	
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
