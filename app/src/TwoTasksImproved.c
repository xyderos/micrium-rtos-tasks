#include <stdio.h>
#include "includes.h"
#include <string.h>

#define DEBUG 0

// Uncomment to test removal of OSTimeDlyHMSM() statements
//#define REMOVE_OSTIMEDLY_HMSM

// Define semaphore
OS_EVENT *criticalSemaphore;

/* Definition of Task Stacks */
/* Stack grows from HIGH to LOW memory */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    stat_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define TASK1_PRIORITY      6  // highest priority
#define TASK2_PRIORITY      7
#define TASK_STAT_PRIORITY 12  // lowest priority 

void printStackSize(char* name, INT8U prio) 
{
    INT8U err;
    OS_STK_DATA stk_data;

    err = OSTaskStkChk(prio, &stk_data);
    if (err == OS_NO_ERR) {
        if (DEBUG == 1)
            printf("%s (priority %d) - Used: %d; Free: %d\n", 
                    name, prio, stk_data.OSUsed, stk_data.OSFree);
    }
    else
    {
        if (DEBUG == 1)
            printf("Stack Check Error!\n");    
    }
}

/* Prints a message and sleeps for given time interval */
void task1(void* pdata)
{
    INT8U err;

    while (1)
    { 
        char text1[] = "Hello from improved Task1\n";
        int i;

        OSSemPend(criticalSemaphore, 0, &err);
        if (err == OS_ERR_NONE) {

            for (i = 0; i < strlen(text1); i++)
                putchar(text1[i]);

            OSSemPost(criticalSemaphore);
        }

#ifndef REMOVE_OSTIMEDLY_HMSM
		// Note: The delay has been increased from the original 11 to 111 in order
		// to make program show display preemptive behaviour (on the DE0-Nano hardware
		// task 2 never got a chance to run otherwise).
        OSTimeDlyHMSM(0, 0, 0, 111); /* Context Switch to next task
                                     * Task will go to the ready state
                                     * after the specified delay
                                     */
#endif
    }
}

/* Prints a message and sleeps for given time interval */
void task2(void* pdata)
{
    INT8U err;
    while (1)
    { 
        char text2[] = "Hello from improved Task2\n";
        int i;

        OSSemPend(criticalSemaphore, 0, &err);
        if (err == OS_ERR_NONE) {
            for (i = 0; i < strlen(text2); i++)
                putchar(text2[i]);

            OSSemPost(criticalSemaphore);
        }

#ifndef REMOVE_OSTIMEDLY_HMSM
            OSTimeDlyHMSM(0, 0, 0, 4);
#endif
    }
}

/* Printing Statistics */
void statisticTask(void* pdata)
{
    while(1)
    {
        printStackSize("Task1", TASK1_PRIORITY);
        printStackSize("Task2", TASK2_PRIORITY);
        printStackSize("StatisticTask", TASK_STAT_PRIORITY);
    }
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
    printf("Lab 3 - Two Tasks Improved\n");

    // initialize semaphore
    criticalSemaphore = OSSemCreate(1); // Initialize with state = 1 (= available)

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
        ( task2,                        // Pointer to task code
          NULL,                         // Pointer to argument passed to task
          &task2_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
          TASK2_PRIORITY,               // Desired Task priority
          TASK2_PRIORITY,               // Task ID
          &task2_stk[0],                // Pointer to bottom of task stack
          TASK_STACKSIZE,               // Stacksize
          NULL,                         // Pointer to user supplied memory (not needed)
          OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
          OS_TASK_OPT_STK_CLR           // Stack Cleared                       
        );  

    if (DEBUG == 1)
    {
        OSTaskCreateExt
            ( statisticTask,                // Pointer to task code
              NULL,                         // Pointer to argument passed to task
              &stat_stk[TASK_STACKSIZE-1],  // Pointer to top of task stack
              TASK_STAT_PRIORITY,           // Desired Task priority
              TASK_STAT_PRIORITY,           // Task ID
              &stat_stk[0],                 // Pointer to bottom of task stack
              TASK_STACKSIZE,               // Stacksize
              NULL,                         // Pointer to user supplied memory (not needed)
              OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
              OS_TASK_OPT_STK_CLR           // Stack Cleared                              
            );
    }  

    OSStart();
    return 0;
}
