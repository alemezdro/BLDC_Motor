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
    

void App_TaskDshot(void *p_arg);

OS_TCB* GetDshotTaskTCB(CPU_VOID);

CPU_STK* GetDshotTaskStk(CPU_VOID);

CPU_VOID SetNewThrottleValue(CPU_INT32U throttle_value);

OS_SEM* GetNewThrottleEventSem(CPU_VOID);

#endif
/* [] END OF FILE */
