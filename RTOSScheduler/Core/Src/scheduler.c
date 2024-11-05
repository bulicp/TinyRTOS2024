/*
 * scheduler.c
 *
 *  Created on: Oct 16, 2024
 *      Author: patriciobulic
 */


#include "scheduler.h"
#include "stm32h7xx_hal.h"

unsigned int stackRegion[NTASKS * TASK_STACK_SIZE];
TCB_Type TCB[NTASKS];
volatile int current_task = 0;


/*
	The function creates a new task. Creating a new task involves:

	1. making space for the task's stack. Each task gets 1kB for the stack
	2. sets the tasks stack pointer in the task table to point at the top of the SW stack frame

	After this steps a new task is ready to be initialized.

 	Pa3cio Bulic, 25.10.2023
*/

void TaskCreate(TCB_Type* pTCB,
		unsigned int* pTaskStackBase,
		void (*TaskFunction)()){

	pTCB->sp 			= (unsigned int*) pTaskStackBase;
	pTCB->pTaskFunction = TaskFunction;
}



void TaskInit(TCB_Type* pTCB){
	HWSF_Type* pHWStackFrame;
	SWSF_Type* pSWStackFrame;


	// Set pointers to HWSF and SWSF:
	pHWStackFrame = (HWSF_Type*)((void*)pTCB->sp - sizeof(HWSF_Type));
	pSWStackFrame = (SWSF_Type*)((void*)pHWStackFrame
                                        - sizeof(SWSF_Type));


	// populate HW Stack Frame
	pHWStackFrame->r0 	= 0;
	pHWStackFrame->r1	= 0x01010101;
	pHWStackFrame->r2 	= 0x02020202;
	pHWStackFrame->r3 	= 0x03030303;
	pHWStackFrame->r12 	= 0x0C0C0C0C;
	pHWStackFrame->lr 	= 0xFFFFFFFF;	// (reset val - task never exits)
	pHWStackFrame->pc 	= (unsigned int) (pTCB->pTaskFunction);
	pHWStackFrame->psr 	= 0x01000000;   // Set T bit (bit 24) in EPSR.
                                        // The Cortex-M4 processor only
                                        // supports execution of
                                        // instructions in Thumb state.
                                        // Attempting to execute
                                        // instructions when the T bit
                                        // is 0 (Debug state)
                                        // 		results in a fault.
	// populate SW Stack Frame
	pSWStackFrame->r4	= 0x04040404;
	pSWStackFrame->r5	= 0x05050505;
	pSWStackFrame->r6	= 0x06060606;
	pSWStackFrame->r7	= 0x07070707;
	pSWStackFrame->r8	= 0x08080808;
	pSWStackFrame->r9	= 0x09090909;
	pSWStackFrame->r10	= 0x0a0a0a0a;
	pSWStackFrame->r11	= 0x0b0b0b0b;

	// Set task's stack pointer in the TCB to point at the top
    // of the task's SW stack frame
	pTCB->sp 			= (unsigned int*) pSWStackFrame;
}


/*
	The function initializes the scheduler.
	Our scheduler is very simple and has fixed number of tasks that are created
	at the start of scheduler.
	The tasks never stop and the number of the running task never changes.

 	Pa3cio Bulic, 25.10.2023
*/

void InitScheduler(unsigned int* pStackRegion, TCB_Type pTCB[], void (*TaskFunctions[])()){
	unsigned int* pTaskStackBase;

	// 1. create all tasks:
	for(int i=0; i<NTASKS; i++){
		pTaskStackBase = pStackRegion + (i+1)*TASK_STACK_SIZE;
		TaskCreate(&pTCB[i], pTaskStackBase, TaskFunctions[i]);
	}

	// 2. initialize all tasks except the Task0.
	//    Task0 will be called by main() and will be the first task interrupted.
	//    Its HWSF and SWSF will be created upon interrupt/contecxt switch
	for(int i=1; i<NTASKS; i++){
		TaskInit(&pTCB[i]);
	}

	// set PSP to Task0.SP:
	__set_PSP((unsigned int)pTCB[0].sp);

}



int ContextSwitch(int current_task, TCB_Type pTCB[]){

	volatile int new_task;


	// 1. shranimo trenutni PSP v pTCB[current_task].sp
	pTCB[current_task].sp = (unsigned int*) __get_PSP();

	// 2. select next task in round-robin fashion
	new_task = current_task + 1;
	if (new_task == NTASKS) new_task = 0;


	// 3. Iz TCB novega opravila preberemo sp in zapišemo v PSP
	__set_PSP((unsigned int)pTCB[new_task].sp);


	return new_task;
}





void Task0(void){
	while(1) {

	}
}

void Task1(void){
	while(1) {
		HAL_Delay(300);
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_3);
	}
}

void Task2(void){
	while(1) {
		HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_13);
		HAL_Delay(600);
	}
}

void Task3(void){
	while(1) {
		HAL_Delay(400);
		HAL_GPIO_TogglePin(GPIOJ, GPIO_PIN_2);
		HAL_Delay(400);
	}
}



