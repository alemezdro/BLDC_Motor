/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include <dshot_task.h>
#include <includes.h>

#include <adc_task.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB App_TaskDShot_TCB;
static CPU_STK App_TaskDShotStk_R[APP_CFG_TASK_DSHOT_STK_SIZE];

static OS_SEM g_sem_buffer_full_event;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          App_TaskDshot()
*
* Description : 
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskDshot()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
void App_TaskDshot(void *p_arg)
{

  /* prevent compiler warnings */
  (void)p_arg;

  OS_ERR os_err_dly;
  OS_ERR os_err_queue;
  OS_ERR os_err_sem;
  
  CPU_TS ts;
  OS_MSG_SIZE cmd_msg_size = 0;

  while (DEF_TRUE)
  {
    //TODO task behavior
    
    //Wait for ´buffer full event
    OSSemPend(&g_sem_buffer_full_event,0,OS_OPT_PEND_BLOCKING,&ts,&os_err_sem);
    
    CPU_INT32U* buffer = getAdcTaskBuffer();
    
    if(NULL != buffer){
      
      //the pointer buffer contains the start of the buffer with the thrust values
      //the buffer length is ADC_TASK_BUFF_LENGTH
      
      //TODO: initialize DMA to send data from (buffer, buffer + ADC_TASK_BUFF_LENGTH) to the 
      //motor controller via the Dshot protocol.
      
      //its possible that the adc task filles the buffers faster than what the dshot task
      //takes to send those values over the motor line. In that case, there should be another
      // semaphore to synchronize the ADC task and let it know that it can continue filling values
    }
    
    // wait for degree value from CMD task. Pending indefinitely.
    CPU_INT32U *com_command = (CPU_INT32U *)OSTaskQPend(10u, OS_OPT_PEND_NON_BLOCKING, &cmd_msg_size, &ts, &os_err_queue);
    
    if(NULL != com_command){
     //TODO: process incomming UART commands
    }
    
    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err_dly);
  }
}

/*
*********************************************************************************************************
*                                          GetDshotTaskTCB()
*
* Description : 
*
* Argument(s) : none
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
OS_TCB* GetDshotTaskTCB()
{
  return &App_TaskDShot_TCB;
}

/*
*********************************************************************************************************
*                                          GetDshotTaskStk()
*
* Description : 
*
* Argument(s) : none
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
CPU_STK* GetDshotTaskStk()
{
  return &App_TaskDShotStk_R[0];
}

OS_SEM* getSemBufferFullEvent()
{
  return &g_sem_buffer_full_event;
}

/* [] END OF FILE */
