#include <stdio.h>
#include "includes.h"
#include <string.h>

// Uncomment to limit the number of iterations
#define LIMIT_ITERATIONS 10

// Define mutex semaphore
OS_EVENT *task0StateSemaphore;
OS_EVENT *task1StateSemaphore;

INT16S* sharedAddress;

/* Definition of Task Stacks */
/* Stack grows from HIGH to LOW memory */
#define   TASK_STACKSIZE       2048
OS_STK    task0_stk[TASK_STACKSIZE];
OS_STK    task1_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define TASK0_PRIORITY      6  // highest priority
#define TASK1_PRIORITY      7

/* Prints a message and sleeps for given time interval */
void task0(void* pdata)
{
    INT8U err;

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

            if (err == OS_ERR_NONE) {

                // inside Task 0 
                if (*sharedAddress < 0) {
                    printf("Received %i\n", *sharedAddress);
                    *sharedAddress = *sharedAddress * -1;
                }

                *sharedAddress = *sharedAddress  + 1;
                printf("Sending %i\n", *sharedAddress);

                // delay for 10 ms
                OSTimeDlyHMSM(0, 0, 0, 10); 

                // signal that task 0 state is now 0 (meaning that the semaphore is state 1)
                OSSemPost(task0StateSemaphore);
            }

#ifdef LIMIT_ITERATIONS
            iterations++;
#endif
        }
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

            if (err == OS_ERR_NONE) {
                if (*sharedAddress > 0) {
                    *sharedAddress = *sharedAddress * -1;
                    // printf("Task 1: Sending %i\n", *sharedAddress);
                }

                // delay for 10ms
                OSTimeDlyHMSM(0, 0, 0, 10);

                // signal that task 1 is in state 1
                OSSemPost(task1StateSemaphore);
            }

#ifdef LIMIT_ITERATIONS
            iterations++;
#endif
        }
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
    printf("Lab 3 - Shared Memory Communication\n");

    // allocate 2 bytes for sharedAddress on the heap
    INT8U err;
    OSMemCreate(sharedAddress, 1, 2, &err);

    // assign initial value of 0
    *sharedAddress = (INT16S) 0;

    // initialize semaphores
    task0StateSemaphore = OSSemCreate(0); // Initialize with state = 0
    task1StateSemaphore = OSSemCreate(1); // Initialize with state = 0

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

    OSStart();
    return 0;
}
