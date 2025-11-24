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

#ifndef THUMBSTICK_TASK_H
#define THUMBSTICK_TASK_H
 
#include <os.h>
#include <cpu.h>

void App_TaskThumbstick(void *p_arg);

OS_TCB* GetThumbstickTaskTCB(CPU_VOID);

CPU_STK* GetThumbstickTaskStk(CPU_VOID);

#endif

/* [] END OF FILE */
