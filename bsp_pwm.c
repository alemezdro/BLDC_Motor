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

#include <bsp_pwm.h>
#include <PWM_1.h>

CPU_VOID pwm_set_interrupt_mode(CPU_INT08U interrupt_mode){
  PWM_1_SetInterruptMode(interrupt_mode);
}

CPU_VOID pwm_write_period(CPU_INT08U period){
  PWM_1_WritePeriod(period);
}

CPU_VOID pwm_write_compare_2(CPU_INT08U compare_value){
  PWM_1_WriteCompare2(compare_value);
}
/* [] END OF FILE */
