#include "ucos_ii.h"
#include <stdio.h>
#include "includes.h"
#include <string.h>
#include "altera_avalon_performance_counter.h"

#define MEASURE_SEMAPHORE_POST_PEND 1
#define MEASURE_CONTEXT_SWITCH_0_TO_1 2
#define MEASURE_CONTEXT_SWITCH_1_TO_0 3

// Uncomment to limit the number of iterations
#define LIMIT_ITERATIONS 10

// Define mutex semaphore
OS_EVENT *task0StateSemaphore;
OS_EVENT *task1StateSemaphore;
OS_EVENT *measurementSemaphore;

/* Definition of Task Stacks */
/* Stack grows from HIGH to LOW memory */
#define   TASK_STACKSIZE       2048
OS_STK    task0_stk[TASK_STACKSIZE];
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    measurement_results_task_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define MEASUREMENT_RESULTS_TASK_PRIORITY      4
#define TASK0_PRIORITY      6
#define TASK1_PRIORITY      7

/* Prints a message and sleeps for given time interval */
void task0(void* pdata)
{
	INT8U err;
	/*
	   Task		SemaphoreValue	State
	   0		0				0	
	   0		1				1
	   1		0				1
	   1		1				0
	   */

#ifdef LIMIT_ITERATIONS
	int iterations = 0;
	while (iterations < LIMIT_ITERATIONS)
#else
		while (1)
#endif 
		{ 
			// wait for task 1 state semaphore to become 1
			// which means that task 1 is in state 0
			// OSSemPend will decrease the semaphore to 0 (= "state" 1)
			OSSemPend(task1StateSemaphore, 0, &err);

            // End performance counter measurement of context switch from 1 to 0
            PERF_END (PERFORMANCE_COUNTER_0_BASE, MEASURE_CONTEXT_SWITCH_1_TO_0);

			printf("Task 1 - State 1\n");

			if (err == OS_ERR_NONE) { // signal that task 0 state is now 0 (meaning that the semaphore is state 1)
				printf("Task 0 - State 1\n");
				// start the timer for measuring the switch time
				OSTimeDlyHMSM(0, 0, 0, 100);

                //  Performance Counter macro to begin timing of the context switch from task 0 to 1.
                PERF_BEGIN (PERFORMANCE_COUNTER_0_BASE, MEASURE_CONTEXT_SWITCH_0_TO_1);
				OSSemPost(task0StateSemaphore);
			}

#ifdef LIMIT_ITERATIONS
			iterations++;
#endif
		}
    
    OSSemPost(measurementSemaphore);
}

void task1(void* pdata)
{
	INT8U err;

#ifdef LIMIT_ITERATIONS
	int iterations = 0;
	while (iterations < LIMIT_ITERATIONS)
#else
		while (1)
#endif 
		{ 
			// wait for task 0 state semaphore to become 1 (task 0 state 1)
			OSSemPend(task0StateSemaphore, 0, &err);

            // End performance counter measurement of context switch from 0 to 1
            PERF_END (PERFORMANCE_COUNTER_0_BASE, MEASURE_CONTEXT_SWITCH_0_TO_1);

			printf("Task 0 - State 0\n");

			if (err == OS_ERR_NONE) {
				// delay for 10ms
				OSTimeDlyHMSM(0, 0, 0, 10);

				// signal that task 1 is in state 1
				printf("Task 1 - State 0\n");

                //  Performance Counter macro to begin timing of the context switch from task 0 to 1.
                PERF_BEGIN (PERFORMANCE_COUNTER_0_BASE, MEASURE_CONTEXT_SWITCH_1_TO_0);
				OSSemPost(task1StateSemaphore);
			}

#ifdef LIMIT_ITERATIONS
			iterations++;
#endif
		}

    OSSemPost(measurementSemaphore);
}

void measurementResultsTask(void* pdata) {

	INT8U err;
    // Wait for all tasks to be done before printing the results
    printf("Waiting for measurements..\n");
    OSSemPend(measurementSemaphore, 0, &err);
    printf("Measurements done, printing results:\n");

    PERF_STOP_MEASURING (PERFORMANCE_COUNTER_0_BASE);
    perf_print_formatted_report( 
        (void *)PERFORMANCE_COUNTER_0_BASE, // Peripheral's HW base address            
        ALT_CPU_FREQ,             // defined in "system.h"
        3,                        // How many sections to print
        "Sem. Post/Pend",         // Display-names of sections
        "Task 0 to 1",         // Display-names of sections
        "Task 1 to 0"         // Display-names of sections
        );  

    alt_u64 averageSemPostPendCycles = perf_get_section_time((void*)PERFORMANCE_COUNTER_0_BASE, MEASURE_SEMAPHORE_POST_PEND) / perf_get_num_starts((void*)PERFORMANCE_COUNTER_0_BASE, MEASURE_SEMAPHORE_POST_PEND);
    alt_u64 averageTask0to1ContextSwitchCycles = perf_get_section_time((void*)PERFORMANCE_COUNTER_0_BASE, MEASURE_CONTEXT_SWITCH_0_TO_1) / perf_get_num_starts((void*)PERFORMANCE_COUNTER_0_BASE, MEASURE_CONTEXT_SWITCH_0_TO_1);
    alt_u64 averageTask1to0ContextSwitchCycles = perf_get_section_time((void*)PERFORMANCE_COUNTER_0_BASE, MEASURE_CONTEXT_SWITCH_1_TO_0) / perf_get_num_starts((void*)PERFORMANCE_COUNTER_0_BASE, MEASURE_CONTEXT_SWITCH_1_TO_0);

    alt_u64 averageContextSwitchCycles = (averageTask0to1ContextSwitchCycles + averageTask1to0ContextSwitchCycles) / 2;
    printf("\nContext switch average no. of CPU cycles: %i\n", (int)averageContextSwitchCycles);
    averageContextSwitchCycles -=  averageSemPostPendCycles;
    printf("Context switch cpu cycle average minus semaphore post/pending function call cycles: %i\n", (int)averageContextSwitchCycles);
    float averageContextSwitchTime = (float) averageContextSwitchCycles / (float) alt_get_cpu_freq();
    printf("Average context switch time: %fs (%fus)\n", averageContextSwitchTime, averageContextSwitchTime * 1000000);
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
	printf("Lab 3 - Handshake\n");

    // Reset (initialize to zero) all section counters and the global 
    // counter of the performance_counter peripheral.
    PERF_RESET (PERFORMANCE_COUNTER_0_BASE);
    PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);

	// initialize semaphores
	task0StateSemaphore = OSSemCreate(0); // Initialize with state = 0
	task1StateSemaphore = OSSemCreate(1); // Initialize with state = 1
    measurementSemaphore = OSSemCreate(0); // When updated, print the measurement results
    

	INT8U err;

    int x = 0;
    for (x = 0; x < 100; x++) {
        PERF_BEGIN (PERFORMANCE_COUNTER_0_BASE, MEASURE_SEMAPHORE_POST_PEND);
        OSSemPost(task0StateSemaphore);
        OSSemPend(task0StateSemaphore, 0, &err);
        PERF_END (PERFORMANCE_COUNTER_0_BASE, MEASURE_SEMAPHORE_POST_PEND);
    }

    // measure semaphore function calls

	OSTaskCreateExt
		( task0,                        // Pointer to task code
		  NULL,                         // Pointer to argument passed to task
		  &task0_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
		  TASK0_PRIORITY,               // Desired Task priority
		  TASK0_PRIORITY,               // Task ID
		  &task0_stk[0],                // Pointer to bottom of task stack
		  TASK_STACKSIZE,               // Stacksize
		  NULL,                         // Pointer to user supplied memory (not needed)
		  OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
		  OS_TASK_OPT_STK_CLR           // Stack Cleared                                 
		);

	OSTaskCreateExt
		( task1,                        // Pointer to task code
		  NULL,                         // Pointer to argument passed to task
		  &task1_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
		  TASK1_PRIORITY,               // Desired Task priority
		  TASK1_PRIORITY,               // Task ID
		  &task1_stk[0],                // Pointer to bottom of task stack
		  TASK_STACKSIZE,               // Stacksize
		  NULL,                         // Pointer to user supplied memory (not needed)
		  OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
		  OS_TASK_OPT_STK_CLR           // Stack Cleared                       
		);  

	OSTaskCreateExt
		( measurementResultsTask,                          // Pointer to task code
		  NULL,                                            // Pointer to argument passed to task
		  &measurement_results_task_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
		  MEASUREMENT_RESULTS_TASK_PRIORITY,               // Desired Task priority
		  MEASUREMENT_RESULTS_TASK_PRIORITY,               // Task ID
		  &measurement_results_task_stk[0],                // Pointer to bottom of task stack
		  TASK_STACKSIZE,                                  // Stacksize
		  NULL,                                            // Pointer to user supplied memory (not needed)
		  OS_TASK_OPT_STK_CHK |                            // Stack Checking enabled 
		  OS_TASK_OPT_STK_CLR                              // Stack Cleared                       
		);  

    printf("Started...\n");

	OSStart();

	return 0;
}
