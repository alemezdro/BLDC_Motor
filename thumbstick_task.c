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

#define BUFFER_INACTIVE -1
#define BUFFER_FULL     ADC_TASK_BUFF_LENGTH

#define MOTOR_THROTTLE_OFF     0u
#define MOTOR_THROTTLE_MIN    48u
#define MOTOR_THROTTLE_MAX    2047u

#define MOTOR_DEADBAND 0.05f

#define ADC_THUMBSTICK_FULL_DOWN    0u
#define ADC_THUMBSTICK_MIDDLE_POINT 2044u
#define ADC_THUMBSTICK_FULL_UP      4092u

#define ADC_MAX_DELTA_UP    (ADC_THUMBSTICK_FULL_UP - ADC_THUMBSTICK_MIDDLE_POINT)
#define ADC_MAX_DELTA_DOWN  (ADC_THUMBSTICK_MIDDLE_POINT - ADC_THUMBSTICK_FULL_DOWN)

#define MIN(a,b) (((a) < (b)) ? (a) : (b)) //call only for integers
#define MAX(a,b) (((a) > (b)) ? (a) : (b)) //call only for integers

static OS_TCB App_TaskAdc_TCB;
static CPU_STK App_TaskAdcStk_R[APP_CFG_TASK_ADC_STK_SIZE];

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static CPU_INT32U g_buff_thrust_values_one[ADC_TASK_BUFF_LENGTH];
static CPU_INT32U g_buff_thrust_values_two[ADC_TASK_BUFF_LENGTH];

static CPU_INT32U* g_active_buff = NULL;

static CPU_INT08S g_idx_buff_one = 0;
static CPU_INT08S g_idx_buff_two = BUFFER_INACTIVE;

static CPU_INT32U g_max_adc_delta = 0;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_INT32U getThumbStickYAxisValue();
CPU_INT32U getMotorThrottleValue(CPU_INT32U y_axis_adc_val);



CPU_INT32U getThumbStickYAxisValue()
{
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

CPU_INT32U getMotorThrottleValue(CPU_INT32U y_axis_adc_val) 
{
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
void App_TaskAdc(void *p_arg)
{
  /* prevent compiler warnings */
  (void)p_arg;

  CPU_TS ts;
  OS_ERR os_err_dly;
  OS_ERR os_err_queue;
  OS_ERR os_err_sem;
  
  volatile CPU_INT32U y_axis_adc_val = 0;
  CPU_INT32U throttle_val = 0;
  
  g_max_adc_delta = ADC_MAX_DELTA_UP > ADC_MAX_DELTA_DOWN ? ADC_MAX_DELTA_UP : ADC_MAX_DELTA_DOWN;

  while (DEF_TRUE)
  {
    //get adc value
    y_axis_adc_val = getThumbStickYAxisValue();
    
    //calculate throttle based on adc
    throttle_val = getMotorThrottleValue(y_axis_adc_val);
    
    if(BUFFER_INACTIVE == g_idx_buff_two){
      //filling buffer 1
      g_buff_thrust_values_one[g_idx_buff_one] = throttle_val;
      g_idx_buff_one++;
    
    }else if(BUFFER_INACTIVE == g_idx_buff_one){
    //filling buffer 2
      g_buff_thrust_values_two[g_idx_buff_two] = throttle_val;
      g_idx_buff_two++;
    }
   
    //one buffer is full already, notify Dshot task and proceed to fill the other buffer
    //there is no need to be notified when dshot empties its queue, because meanwhile this
    //task fills the other buffer
    if(BUFFER_FULL == g_idx_buff_one || BUFFER_FULL == g_idx_buff_two){
      
      OSSemPost(getSemBufferFullEvent(),OS_OPT_POST_ALL,&os_err_sem);
      
      if(BUFFER_FULL == g_idx_buff_one){
        
        g_active_buff = &g_buff_thrust_values_one[0];
        
        g_idx_buff_one = BUFFER_INACTIVE; //deactivate buffer because it being emptied
        g_idx_buff_two = 0;
      
      }else if(BUFFER_FULL == g_idx_buff_two){
        
        g_active_buff = &g_buff_thrust_values_two[0];
      
        g_idx_buff_two = BUFFER_INACTIVE; //deactivate buffer because it being emptied
        g_idx_buff_one = 0;
      }
      
      }

    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err_dly);
  }
}

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
OS_TCB* GetAdcTaskTCB()
{
  return &App_TaskAdc_TCB;
}

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
CPU_STK* GetAdcTaskStk()
{
  return &App_TaskAdcStk_R[0];
}


CPU_INT32U* getAdcTaskBuffer()
{
  CPU_INT32U* buff = g_active_buff;
  
  //invalidate active buffer to avoid caller to reprocess the same data again
  
  g_active_buff = NULL; 

  return buff;
}
/* [] END OF FILE */
