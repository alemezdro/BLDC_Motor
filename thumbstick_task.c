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
#include <thumbstick_task.h>
#include <includes.h>

#include <project.h>

#include <dshot_task.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define SPI_TX_START_BYTE (CPU_INT08U)0b00000110
#define SPI_TX_LAST_BYTE (CPU_INT08U)0b00000000

#define SPI_CH0 0
#define SPI_CH1 1
#define SPI_CH2 2

#define SPI_THUMBSTICK_Y     SPI_CH0
#define SPI_THUMBSTICK_X     SPI_CH1
#define SPI_THUMBSTICK_PRESS SPI_CH2

#define SPI_CH_SELECT(x) (((x) & 0x03) << 6)

#define SPI_RX_START_BYTE_POS 16
#define SPI_RX_MSB_BYTE_POS   8
#define SPI_RX_LSB_BYTE_POS   0

#define SPI_THUMBSTICK_RX_MASK 0xFFF

#define MOTOR_DEADBAND 0.05f

#define ADC_THUMBSTICK_FULL_DOWN    0u
#define ADC_THUMBSTICK_MIDDLE_POINT 2044u
#define ADC_THUMBSTICK_FULL_UP      4092u

#define ADC_MAX_DELTA_UP    (ADC_THUMBSTICK_FULL_UP - ADC_THUMBSTICK_MIDDLE_POINT)
#define ADC_MAX_DELTA_DOWN  (ADC_THUMBSTICK_MIDDLE_POINT - ADC_THUMBSTICK_FULL_DOWN)

#define MIN(a,b) (((a) < (b)) ? (a) : (b)) //call only for integers
#define MAX(a,b) (((a) > (b)) ? (a) : (b)) //call only for integers

#define MAX_THROTTLE_STEP 150

static OS_TCB App_TaskAdc_TCB;
static CPU_STK App_TaskAdcStk_R[APP_CFG_TASK_THUMBSTICK_STK_SIZE];

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static CPU_INT32U g_max_adc_delta = 0;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_INT32U GetThumbStickYAxisValue(CPU_VOID);

CPU_INT32U GetMotorThrottleValue(CPU_INT32U y_axis_adc_val);

CPU_INT32S GetMotorThrottleValueRegulated(CPU_INT32S last_throttle_value, CPU_INT32S throttle_value);

/*
*********************************************************************************************************
*                                          App_TaskThumbstick()
*
* Description : The thumbstick task reads periodically the ADC value from the thumbstick component via
*               SPI, maps the ADC value to a valid Dshot range and if the new value differs from the
*               last measured one, it notifies the Dshot task that a new value is available by the use
*               of a semaphore.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskThumbstick()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
void App_TaskThumbstick(void *p_arg){
    
  /* prevent compiler warnings */
  (void)p_arg;

  OS_ERR os_err_dly;
  OS_ERR os_err_sem;

  CPU_INT32U y_axis_adc_val = 0;

  CPU_INT32S last_motor_throttle_val = 0;
  
  g_max_adc_delta = ADC_MAX_DELTA_UP > ADC_MAX_DELTA_DOWN ? ADC_MAX_DELTA_UP : ADC_MAX_DELTA_DOWN;

  while (DEF_TRUE){
    //get adc value only for Y axis
    y_axis_adc_val = GetThumbStickYAxisValue();
    
    //GetMotorThrottleValue() always returns max MOTOR_THROTTLE_MAX
    
    CPU_INT32S current_throttle = GetMotorThrottleValueRegulated(last_motor_throttle_val, GetMotorThrottleValue(y_axis_adc_val));
    
    if(last_motor_throttle_val != current_throttle){
        
        //calculate throttle based on adc and send it to the Dshot 
        SetNewThrottleValue(current_throttle);
        
        last_motor_throttle_val = current_throttle;
        
        //Notify the Dshot task on new throttle value event
        OSSemPost(GetNewThrottleEventSem(),OS_OPT_POST_ALL,&os_err_sem);
    }
  
    OSTimeDlyHMSM(0, 0, 0, THUMSTICK_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err_dly);
  }
}

/*
*********************************************************************************************************
*                                          GetThumbStickYAxisValue()
*
* Description : Measures only the Y axis adc value from the thumbstick component (0...4095). For the
*               measuremement is communicates with the thumbstick via SPI.
*
* Argument(s) : none
*
* Return(s)   : Measured ADC value for the thumstick Y-Axis
*
* Note(s)     : none
*********************************************************************************************************
*/
CPU_INT32U GetThumbStickYAxisValue(CPU_VOID){
    
  CPU_INT32U result = 0;
  
  CS_1_Write(0); // CS selection
  
  SPIM_1_WriteTxData(SPI_TX_START_BYTE);
  SPIM_1_WriteTxData(SPI_CH_SELECT(SPI_THUMBSTICK_Y));
  SPIM_1_WriteTxData(SPI_TX_LAST_BYTE);
  
  while (!(SPIM_1_ReadTxStatus() & SPIM_1_STS_SPI_DONE));  // wait until TX done
  
  /*read thumbstick response. It consist of 3 bytes out of which only the last 12
  bits of the last 2 bytes correspond to the ADC measurement*/
  
  result = SPIM_1_ReadRxData() << SPI_RX_START_BYTE_POS;
  result |= SPIM_1_ReadRxData() << SPI_RX_MSB_BYTE_POS;
  result |= SPIM_1_ReadRxData() << SPI_RX_LSB_BYTE_POS;

  CS_1_Write(1); //CS deselection
  
  result &= SPI_THUMBSTICK_RX_MASK; //mask relevant
  
  return result;
}

/*
*********************************************************************************************************
*                                          GetMotorThrottleValue()
*
* Description : Converts an ADC value (in this case from the Y-Axis into a suitable dshot motor throttle
*               value. It can be either 0 or [MOTOR_THROTTLE_MIN...MOTOR_THROTTLE_MAX].
*
* Argument(s) : y_axis_adc_val variable containing the Y-Axis adc value
*
* Return(s)   : throttle value derived from y_axis_adc_val
*
* Note(s)     : none
*********************************************************************************************************
*/
CPU_INT32U GetMotorThrottleValue(CPU_INT32U y_axis_adc_val){
    
  //distance from the center (aboslute value)
  CPU_FP32 delta = fabsf((CPU_FP32)y_axis_adc_val - (CPU_FP32)ADC_THUMBSTICK_MIDDLE_POINT);
  
  //normalize 0..1: 0 = center, 1 = at either end
  CPU_FP32 normed_val = delta / (CPU_FP32)g_max_adc_delta;
  
  //safety clamp
  normed_val = fminf(normed_val, 1.0f);
  
  //apply deadband. Slow motions remains 0. Beyond 5% 0..1
   if(normed_val < MOTOR_DEADBAND){
    normed_val = 0.0f; //normed value too small. Do not move the motor
  }else{
    normed_val = (normed_val - MOTOR_DEADBAND)/(1.0f - MOTOR_DEADBAND);
    
    //safety clamp    
    normed_val = fminf(normed_val, 1.0f);
  }
  
  CPU_INT32U throttle = MOTOR_THROTTLE_OFF;
  
  if(0.0f != normed_val){
    
    throttle = MOTOR_THROTTLE_MIN + (CPU_INT32U)lroundf(normed_val * (MOTOR_THROTTLE_MAX - MOTOR_THROTTLE_MIN));
    
    throttle = MAX(throttle, MOTOR_THROTTLE_MIN);
    throttle = MIN(throttle, MOTOR_THROTTLE_MAX);
  }

  return throttle;
}

/*
*********************************************************************************************************
*                                          GetMotorThrottleValueRegulated()
*
* Description : Regulate the throttle value. It determines with the help of the throttle value of the last
*               iteration and the current throttle value, if the throttle increase/decrease magnitude falls
*               within the desired range [-MAX_THROTTLE_STEP : MAX_THROTTLE_STEP]. If the mangnitude is outside
*               the range, limit the increase/decrease to MAX_THROTTLE_STEP/-MAX_THROTTLE_STEP. This way the
*               motor can be driven softly when the thumbstick moves too fast, avoiding sudden high current
*               consumption.
*
* Argument(s) : last_throttle_value: variable containing the throttle value of the last interation
*               throttle_value:      variable containing the throttle value of the current iteration
*
* Return(s)   : Throttle value that will be send to the dshot task to operate the motor.
*
* Note(s)     : none
*********************************************************************************************************
*/
CPU_INT32S GetMotorThrottleValueRegulated(CPU_INT32S last_throttle_value, CPU_INT32S throttle_value){
    
    CPU_INT32S throttle_diff = 0;
    CPU_INT32S regulated_throttle_value = throttle_value;
    
    if(last_throttle_value != throttle_value){
        
        throttle_diff = throttle_value - last_throttle_value;
        
        if(throttle_diff > MAX_THROTTLE_STEP){
            throttle_diff = MAX_THROTTLE_STEP;
        }else if(throttle_diff < (MAX_THROTTLE_STEP * -1)){
            throttle_diff = MAX_THROTTLE_STEP * -1;
        }
        
        regulated_throttle_value = last_throttle_value + throttle_diff;
    } 
    
    return regulated_throttle_value;
}

/*
*********************************************************************************************************
*                                          GetThumbstickTaskTCB()
*
* Description : Get Thumbstick task TCB block
*
* Argument(s) : none
*
* Return(s)   : Pointer to the Thumbstick task TCB block
*
* Note(s)     : none
*********************************************************************************************************
*/
OS_TCB* GetThumbstickTaskTCB(CPU_VOID){
  return &App_TaskAdc_TCB;
}

/*
*********************************************************************************************************
*                                          GetThumbstickTaskStk()
*
* Description : Get Thumbstick Task stack
*
* Argument(s) : none
*
* Return(s)   : Pointer to the Thumbstick task stack
*
* Note(s)     : none
*********************************************************************************************************
*/
CPU_STK* GetThumbstickTaskStk(CPU_VOID){
  return &App_TaskAdcStk_R[0];
}
/* [] END OF FILE */
