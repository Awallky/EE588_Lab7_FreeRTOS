#include "accelerometer_task.h"

#define UNLOCKED 0
#define LOCKED 1
extern void checkbuttons(void);

extern uint16_t Red, Green, Blue;
extern uint16_t JoyX, JoyY;
extern uint16_t AccX, AccY, AccZ;


uint32_t Accelerometer_Init(void){
	BSP_Accelerometer_Init();
	
	//
	// Create the LED task.
	//
	if(xTaskCreate(monitor_accelerometers, (const portCHAR *)"Accelerometer", ACCELEROMETERTASKSIZE, NULL,
								 tskIDLE_PRIORITY + PRIORITY_ACCELEROMETER_TASK, NULL) != pdTRUE)
	{
			return(1);
	}

	//
	// Success.
	//
	return(0);
}

static void monitor_accelerometers(void){
	while(1){
		BSP_Accelerometer_Input(&AccX, &AccY, &AccZ);
	
		if((AccX < 325) && (AccY > 325) && (AccY < 675) && (AccZ > 325) && (AccZ < 675)){
				Red = 500; Green = 0; Blue = 0;
		} 
		else if((AccY < 325) && (AccX > 325) && (AccX < 675) && (AccZ > 325) && (AccZ < 675)){
				Red = 350; Green = 350; Blue = 0;
		} 
		else if((AccX > 675) && (AccY > 325) && (AccY < 675) && (AccZ > 325) && (AccZ < 675)){
				Red = 0; Green = 500; Blue = 0;
		} 
		else if((AccY > 675) && (AccX > 325) && (AccX < 675) && (AccZ > 325) && (AccZ < 675))
		{
				Red = 0; Green = 0; Blue = 500;
		}
		else{
			;
		}
	}
}
