//*****************************************************************************
//
// freertos_demo.c - Simple FreeRTOS example.
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

//*****************************************************************************
// 	Valvano's Functions that are incompatible due to shared usage of Ports:
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "./../include/inc/hw_memmap.h"
#include "./../include/inc/hw_types.h"
#include "./../include/driverlib/gpio.h"
#include "./../include/driverlib/pin_map.h"
#include "./../include/driverlib/rom.h"
#include "./../include/driverlib/sysctl.h"
#include "./../include/driverlib/uart.h"
#include "./../include/utils/uartstdio.h"
#include "led_task.h"
#include "switch_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "BSP.h"
#include "accelerometer_task.h"

#define SW1_TEXT_MODE 	0
#define SW1_BUBBLE_MODE 1
#define SW2_UNLOCKED		0
#define SW2_LOCKED			1
#define MKII_SW1				0x30
#define MKII_SW2				0x40
uint8_t sw1_mode, sw2_mode;
uint8_t store_prev_accel, prev_accel_stored, need_to_print;
uint8_t just_unlocked;

//*****************************************************************************
//
// The mutex that protects concurrent access of UART from multiple tasks.
//
//*****************************************************************************
xSemaphoreHandle g_pUARTSemaphore;

//*****************************************************************************
//
// The mutex that protects concurrent access of sw1_mode and sw2_mode from 
// multiple tasks.
//
//*****************************************************************************
xSemaphoreHandle g_pAccelerometerSemaphore, g_pModeSemaphore;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}

#endif

//*****************************************************************************
//
// Valvano MKII Booster Pack Example Code
//
//*****************************************************************************
uint16_t Red = 0, Green = 0, Blue = 0;
uint16_t JoyX, JoyY;
uint16_t AccX, AccY, AccZ;
uint16_t prev_AccX, prev_AccY, prev_AccZ;
uint8_t current;

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
// This hook is called by FreeRTOS when an stack overflow error is detected.
//
//*****************************************************************************
void
vApplicationStackOverflowHook(xTaskHandle *pxTask, char *pcTaskName)
{
    //
    // This function can not return, so loop forever.  Interrupts are disabled
    // on entry to this function, so no processor interrupts will interrupt
    // this loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************
void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// Initialize FreeRTOS and start the initial set of tasks.
//
//*****************************************************************************
int
main(void)
{
		sw1_mode = SW1_TEXT_MODE;
		sw2_mode = SW2_UNLOCKED;
		prev_AccX = 0;
		prev_AccY = 0;
		prev_AccZ = 0;
		just_unlocked = 0;
    //
    // Set the clocking to run at 50 MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Initialize the UART and configure it for 115,200, 8-N-1 operation.
    //
    ConfigureUART();

    //
    // Print demo introduction.
    //
    UARTprintf("\n\nWelcome to the EK-TM4C123GXL FreeRTOS Demo!\n");

    //
    // Create a mutex to guard the UART.
    //
    g_pUARTSemaphore = xSemaphoreCreateMutex();
		g_pAccelerometerSemaphore = xSemaphoreCreateMutex();
		//g_pSW2Semaphore = xSemaphoreCreateMutex();

    //
    // Create the LED task.
    //
    if(LEDTaskInit() != 0)
    {

        while(1)
        {
        }
    }

    //
    // Create the switch task.
    //
    if(SwitchTaskInit() != 0)
    {

        while(1)
        {
        }
    }
		
		//
    // Create the Accelerometer task.
    //
    if(AccelerometerTaskInit() != 0)
    {

        while(1)
        {
        }
    }
		
    //
    // Start the scheduler.  This should not return.
    //
    vTaskStartScheduler();

    //
    // In case the scheduler returns for some reason, print an error and loop
    // forever.
    //

    while(1)
    {
    }
}
