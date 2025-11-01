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

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/
static OS_TCB App_TaskDShot_TCB;
static CPU_STK App_TaskDShotStk_R[APP_CFG_TASK_DSHOT_STK_SIZE];

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/


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

  OS_ERR os_err;
  CPU_TS ts;

  while (DEF_TRUE)
  {
    //TODO task behavior
    
    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
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

/* [] END OF FILE */
