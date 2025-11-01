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

#ifndef ADC_TASK_H
#define ADC_TASK_H
 
#include <os.h>
#include <cpu.h>

/*
*********************************************************************************************************
*                                          App_TaskAdc()
*
* Description : 
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskAdc()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
void App_TaskAdc(void *p_arg);

/*
*********************************************************************************************************
*                                          GetAdcTaskTCB()
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
OS_TCB* GetAdcTaskTCB();

/*
*********************************************************************************************************
*                                          GetAdcTaskStk()
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
CPU_STK* GetAdcTaskStk();

#endif

/* [] END OF FILE */
