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

#include <adc_task.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define REMOVE_CODE 1
/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB App_TaskDShot_TCB;
static CPU_STK App_TaskDShotStk_R[APP_CFG_TASK_DSHOT_STK_SIZE];

static OS_SEM g_sem_buffer_full_event;

static CPU_INT08U g_pwm_enable_value;
static CPU_INT08U g_pwm_disable_value;

static CPU_INT08U g_cmp_values[20];

static CPU_INT08U g_td0;
static CPU_INT08U g_td1;
static CPU_INT08U g_td2;
static CPU_INT08U g_td3;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
CPU_VOID configure_pwm(CPU_VOID);

CPU_INT08U configure_dma(CPU_VOID);

CPU_VOID enable_dma(CPU_INT08U dma_channel);

CPU_VOID wait_td_finish(CPU_INT08U dma_channel);

CPU_VOID wait_td_chain_finish(CPU_INT08U dma_channel);

CPU_VOID configure_pwm(CPU_VOID){
    
    pwm_set_interrupt_mode(0);
    
    pwm_write_compare_2(20);
    
    //dma enables the pwm channel
    //only cmp channel 2 changes here (ch1 with 100 remain)
    //no period changed at run 
}

CPU_INT08U configure_dma(CPU_VOID){
    
    g_pwm_enable_value = PWM_1_CONTROL |= PWM_1_CTRL_ENABLE;
    g_pwm_disable_value = PWM_1_CONTROL &= ((uint8)(~PWM_1_CTRL_ENABLE));
    
    for(CPU_INT08U i = 0; i < 20; i++){
        if(i % 2 == 0){
            g_cmp_values[i] = 39;
        }else{
            g_cmp_values[i] = 99;
        }
    }

    //This is the dma configuration that's done once
    
    CPU_INT08U dma_channel = init_dma(1u, 1u, HI16(g_cmp_values), HI16(PWM_1_COMPARE1_LSB_PTR));
    
    g_td0 = dma_td_allocate();
    g_td1 = dma_td_allocate();
    g_td2 = dma_td_allocate();
    g_td3 = dma_td_allocate();
    
    //setup first compare value
    dma_td_set_configuration(g_td0, 1u, g_td1, CY_DMA_TD_AUTO_EXEC_NEXT); //1 byte in total, auto execute the next TD
    
    dma_td_set_address(g_td0, LO16((CPU_INT32U)&g_cmp_values[0]), LO16((CPU_INT32U)PWM_1_COMPARE1_LSB_PTR));
    
    //enable PWM output
    dma_td_set_configuration(g_td1, 1u, g_td2, 0u); //1 byte in total, no auto execute next td
    
    dma_td_set_address(g_td1, LO16((CPU_INT32U)&g_pwm_enable_value), LO16((CPU_INT32U)PWM_1_CONTROL_PTR));  
    
    //setup rest of cmp values
    dma_td_set_configuration(g_td2, 19u, g_td3, CY_DMA_TD_INC_SRC_ADR); //1 byte in total, auto execute the next TD
    
    dma_td_set_address(g_td2, LO16((CPU_INT32U)&g_cmp_values[1]), LO16((CPU_INT32U)PWM_1_COMPARE1_LSB_PTR));    
    
    //Disable PWM output
    dma_td_set_configuration(g_td3, 1u, CY_DMA_DISABLE_TD, 0u); //1 byte in total, no auto execute next td
    
    dma_td_set_address(g_td3, LO16((CPU_INT32U)&g_pwm_disable_value), LO16((CPU_INT32U)PWM_1_CONTROL_PTR));     
    
    //Do not enable channel yet. Task does this
    return dma_channel;
}

CPU_VOID enable_dma(CPU_INT08U dma_channel){

    dma_ch_set_init_td(dma_channel, g_td0);
    dma_ch_enable(dma_channel, 1u); //enables dma channel and it preserves td config after td chain finished
}

CPU_VOID wait_td_finish(CPU_INT08U dma_channel){
    
    CPU_INT08U current_td = 0;
    CPU_INT08U state = 0;

    do{
        dma_ch_status(dma_channel, &current_td, &state);
    }while(state & CY_DMA_STATUS_TD_ACTIVE);
}

CPU_VOID wait_td_chain_finish(CPU_INT08U dma_channel){
    
    CPU_INT08U current_td = 0;
    CPU_INT08U state = 0;

    do{
        dma_ch_status(dma_channel, &current_td, &state);
    }while(state & CY_DMA_STATUS_CHAIN_ACTIVE);
}
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

  OS_ERR os_err_dly;
  OS_ERR os_err_queue;
  OS_ERR os_err_sem;
  
  CPU_TS ts;
  OS_MSG_SIZE cmd_msg_size = 0;

  configure_pwm();

  CPU_INT08U dma_channel = configure_dma();

  while (DEF_TRUE)
  {
    //TODO task behavior
    
    enable_dma(dma_channel);
    
    //trigger the TD chain only once. After that the PWM channel 2 should trigger DMA
    dma_ch_set_request(dma_channel, CY_DMA_CPU_REQ);
    
    wait_td_chain_finish(dma_channel); //TASK SHOULD NOT STALL HERE. Chain should be finished
    
    //Measure output!
    OSTimeDlyHMSM(0, 0, 3, 0, OS_OPT_TIME_HMSM_STRICT, &os_err_dly);
    
#if REMOVE_CODE == 0    
    
    //Wait for ´buffer full event
    OSSemPend(&g_sem_buffer_full_event,0,OS_OPT_PEND_BLOCKING,&ts,&os_err_sem);
    
    CPU_INT32U* buffer = getAdcTaskBuffer();
    
    if(NULL != buffer){
      
      //the pointer buffer contains the start of the buffer with the thrust values
      //the buffer length is ADC_TASK_BUFF_LENGTH
      
      //TODO: initialize DMA to send data from (buffer, buffer + ADC_TASK_BUFF_LENGTH) to the 
      //motor controller via the Dshot protocol.
      
      //its possible that the adc task filles the buffers faster than what the dshot task
      //takes to send those values over the motor line. In that case, there should be another
      // semaphore to synchronize the ADC task and let it know that it can continue filling values
    }
    
    // wait for degree value from CMD task. Pending indefinitely.
    CPU_INT32U *com_command = (CPU_INT32U *)OSTaskQPend(10u, OS_OPT_PEND_NON_BLOCKING, &cmd_msg_size, &ts, &os_err_queue);
    
    if(NULL != com_command){
     //TODO: process incomming UART commands
    }
    
#endif    
    
    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err_dly);
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

OS_SEM* getSemBufferFullEvent()
{
  return &g_sem_buffer_full_event;
}

/* [] END OF FILE */
