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
#include <cpu.h>

CPU_VOID init_pwm(CPU_VOID);

CPU_VOID stop_pwm(CPU_VOID);

CPU_VOID pwm_set_interrupt_mode(CPU_INT08U interrupt_mode);

CPU_VOID pwm_write_period(CPU_INT08U period);

CPU_VOID pwm_write_compare_1(CPU_INT08U compare_value);

CPU_VOID pwm_write_compare_2(CPU_INT08U compare_value);
/* [] END OF FILE */
