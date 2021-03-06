// accelerometer_task.c 

#include "accelerometer_task.h"

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
xQueueHandle g_pACCELEROMETERQueue;

#define SW1_TEXT_MODE 	0
#define SW1_BUBBLE_MODE 1
#define SW2_UNLOCKED		0
#define SW2_LOCKED			1
#define MKII_SW1				0x30
#define MKII_SW2				0x40
extern uint8_t sw1_mode, sw2_mode;
extern uint8_t store_prev_accel, prev_accel_stored, need_to_print;
extern uint16_t Red, Green, Blue;
extern uint16_t JoyX, JoyY;
extern uint16_t AccX, AccY, AccZ;
extern uint16_t prev_AccX, prev_AccY, prev_AccZ;
extern uint8_t current;
extern xSemaphoreHandle g_pUARTSemaphore;
extern xSemaphoreHandle g_pAccelerometerSemaphore, g_pModeSemaphore;
extern xQueueHandle g_pLEDQueue, g_pAccelerometerQueue;

//*****************************************************************************
//
// This task toggles the user selected LED at a user selected frequency. User
// can make the selections by pressing the left and right buttons.
//
//*****************************************************************************
static void
AccelerometerTask(void *pvParameters)
{
		portTickType ui32WakeTime;
    uint32_t ui32LEDToggleDelay;
    uint8_t ui8Message;
		
		//
    // Initialize the LED Toggle Delay to default value.
    //
    ui32LEDToggleDelay = LED_TOGGLE_DELAY;

    //
    // Get the current tick count.
    //
    ui32WakeTime = xTaskGetTickCount();
	
    while(1){
			//
			// Wait for the required amount of time.
			//
			vTaskDelayUntil(&ui32WakeTime, ui32LEDToggleDelay / portTICK_RATE_MS);

			//
			// Turn off the LED.
			//
			//RGBDisable();
			xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
			UARTprintf("X: %u\tY: %u\tZ: %u\n", AccX, AccY, AccZ);
			xSemaphoreGive(g_pUARTSemaphore);
			
			xSemaphoreTake(g_pAccelerometerSemaphore, portMAX_DELAY);
			BSP_Accelerometer_Input(&AccX, &AccY, &AccZ);
			if( store_prev_accel ){
				store_prev_accel = 0;
				prev_AccX = AccX;
				prev_AccY = AccY;
				prev_AccZ = AccZ;
			}
			xSemaphoreGive(g_pAccelerometerSemaphore);
			
			//
			// Wait for the required amount of time.
			//
			vTaskDelayUntil(&ui32WakeTime, ui32LEDToggleDelay / portTICK_RATE_MS);
		}
}

//*****************************************************************************
//
// Initializes the LED task.
//
//*****************************************************************************
uint32_t
AccelerometerTaskInit(void)
{
    //
    // Print the current loggling LED and frequency.
    //
    UARTprintf("\nInitializing the Accelerometer\n");
	
		BSP_Accelerometer_Init();

    //
    // Create a queue for sending messages to the LED task.
    //
    g_pACCELEROMETERQueue = xQueueCreate(ACCELEROMETER_QUEUE_SIZE, ACCELEROMETER_ITEM_SIZE);

    //
    // Create the LED task.
    //
    if(xTaskCreate(AccelerometerTask, (const portCHAR *)"Accelerometer", ACCELEROMETERTASKSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_ACCELEROMETER_TASK, NULL) != pdTRUE)
    {
        return(1);
    }

    //
    // Success.
    //
    return(0);
}
