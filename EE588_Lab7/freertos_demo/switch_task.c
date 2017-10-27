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

extern xQueueHandle g_pLEDQueue;
extern xSemaphoreHandle g_pUARTSemaphore;

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
        ui8CurButtonState = ButtonsPoll(0, 0);

        //
        // Check if previous debounced state is equal to the current state.
        //
        if(ui8CurButtonState != ui8PrevButtonState)
        {
            ui8PrevButtonState = ui8CurButtonState;

            //
            // Check to make sure the change in state is due to button press
            // and not due to button release.
            //
            if((ui8CurButtonState & ALL_BUTTONS) != 0)
            {
                if((ui8CurButtonState & ALL_BUTTONS) == LEFT_BUTTON)
                {
                    ui8Message = LEFT_BUTTON;

                    //
                    // Guard UART from concurrent access.
                    //
                    xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                    UARTprintf("Left Button is pressed.\n");
                    xSemaphoreGive(g_pUARTSemaphore);
										
                }
                else if((ui8CurButtonState & ALL_BUTTONS) == RIGHT_BUTTON)
                {
                    ui8Message = RIGHT_BUTTON;

                    //
                    // Guard UART from concurrent access.
                    //
                    xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                    UARTprintf("Right Button is pressed.\n");
                    xSemaphoreGive(g_pUARTSemaphore);
                }

                //
                // Pass the value of the button pressed to LEDTask.
                //
                if(xQueueSend(g_pLEDQueue, &ui8Message, portMAX_DELAY) !=
                   pdPASS)
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
    ButtonsInit();
	
		//
		// BSP Buttons Init
		//
		
		
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

void checkbuttons(void){
  static uint8_t prev1 = 0, prev2 = 0, prevS = 0; // previous values
  static uint8_t mode = 0;
  uint8_t current;

  //BSP_Buzzer_Set(0);
  //BSP_Joystick_Input(&JoyX, &JoyY, &current);
  BSP_Accelerometer_Input(&AccX, &AccY, &AccZ);
  if((current == 0) && (prevS != 0)){
    // Select was pressed since last loop
    mode = (mode + 1)&0x03;
    Red = Green = Blue = 0;
    //BSP_Buzzer_Set(512);           // beep until next interrupt (0.1 sec beep)
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
  //BSP_RGB_Set(Red, Green, Blue);
}