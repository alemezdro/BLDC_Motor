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
#ifndef COM_TASK_H
#define COM_TASK_H
 
#include <os.h>
#include <cpu.h>
  
void App_TaskCom(void *p_arg);

OS_TCB* GetComTaskTCB(CPU_VOID);

CPU_STK* GetComTaskStk(CPU_VOID);

#endif
/* [] END OF FILE */
