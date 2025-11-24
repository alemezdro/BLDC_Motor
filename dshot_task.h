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
#ifndef DSHOT_TASK_H
#define DSHOT_TASK_H

#include <os.h>
#include <cpu.h>
    
#define MOTOR_THROTTLE_OFF     0u
#define MOTOR_THROTTLE_MIN    48u
//should be 2047 but the motor stops rotating as soons as the throttle surpasses 1000
//possible current limitation in the motor
#define MOTOR_THROTTLE_MAX    1000u
    

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
void App_TaskDshot(void *p_arg);

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
OS_TCB* GetDshotTaskTCB();

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
CPU_STK* GetDshotTaskStk();

CPU_VOID SetNewThrottleValue(CPU_INT32U throttle_value);

OS_SEM* GetNewThrottleEventSem();

#endif
/* [] END OF FILE */
