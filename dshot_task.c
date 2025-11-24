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

#include <thumbstick_task.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define DMA_AMOUNT_TDS 4u

#define DMA_FIRST_CMP_VAL_TD    0u
#define DMA_START_TD            1u
#define DMA_CMP_VALUES_TD       2u
#define DMA_STOP_TD             3u

#define DSHOT_CMP_VALUES_LEN     17u

#define DMA_TD_NO_CONFIGURATION  0u
#define ONE_BYTE                 1u
#define ENABLE_REQ_PER_BURST     1u
#define PRESERVE_TD_CHAIN_CONFIG 1u

#define DSHOT_BIT_1     8u
#define DSHOT_BIT_0     23u

#define RESSOURCE_WAIT_TIME 1u

#define DMA_CH2_CMP_VALUE 18u

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB App_TaskDShot_TCB;
static CPU_STK App_TaskDShotStk_R[APP_CFG_TASK_DSHOT_STK_SIZE];

static OS_SEM       g_sem_new_throttle_event;
static CPU_INT32U   g_current_throttle_value;

 //value to load in the pwm control register to start dma
CPU_INT08U g_pwm_enable_value;
//value to load in the pwm control register to stop dma
CPU_INT08U g_pwm_disable_value;

enum OperationState{
    THUMBSTICK_MODE = 0,
    USER_MODE       = 1
};


static CPU_INT08U g_cmp_values[DSHOT_CMP_VALUES_LEN];
static CPU_INT08U g_td[DMA_AMOUNT_TDS];

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
CPU_VOID configure_pwm(CPU_VOID);

CPU_INT08U configure_dma(CPU_VOID);

CPU_VOID start_dma(CPU_INT08U dma_channel);

CPU_INT16U get_dshot_frame_format(uint32_t throttle_value);

CPU_VOID update_dshot_frame_buffer(uint32_t throttle_value);

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
void App_TaskDshot(void *p_arg) {

  /* prevent compiler warnings */
  (void)p_arg;

  OS_ERR os_err_dly;
  OS_ERR os_err_queue;
  OS_ERR os_err_sem;
  
  CPU_TS ts;
  OS_MSG_SIZE cmd_msg_size = 0;

   volatile CPU_INT32U throttle_val = 0;
   CPU_INT32U* user_throttle_val = NULL;
   enum OperationState state = THUMBSTICK_MODE;

  //pwm intialization
  configure_pwm();

  //dma initialization
  CPU_INT08U dma_channel = configure_dma();

    while (DEF_TRUE) {
 
        //start dma exectuion
        start_dma(dma_channel);
        
        //check semaphore throttle value event
        OSSemPend(&g_sem_new_throttle_event,RESSOURCE_WAIT_TIME,OS_OPT_PEND_NON_BLOCKING,&ts,&os_err_sem);
        
        //check user command buffer
        user_throttle_val = (CPU_INT32U *)OSTaskQPend(RESSOURCE_WAIT_TIME, OS_OPT_PEND_NON_BLOCKING, &cmd_msg_size, &ts, &os_err_queue);
        
        
        if(OS_ERR_NONE == os_err_queue){
            //the call was succesfull and this task owns the ressource
            //user sent something. check it and enter or leave the user mode eventually
            
            if(0 == *user_throttle_val){
                state = THUMBSTICK_MODE; //leave user mode
            
            }else{
                //the task assumes that thet throttle value was correctly controlled by the com task
                //apply the user throttle value
                update_dshot_frame_buffer(*user_throttle_val);
                
                state = USER_MODE; //enter user mode
            }
            
        }else if(OS_ERR_NONE == os_err_sem && THUMBSTICK_MODE == state){
            //the call was succesfull and this task owns the ressource
            //consider thumbstick value only in the thumbstick mode
            throttle_val = g_current_throttle_value; // atomic read
    
            update_dshot_frame_buffer(throttle_val);
        }

        OSTimeDlyHMSM(0, 0, 0, DSHOT_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err_dly); //NO MORE DELAY THAN 15 ms
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

CPU_VOID SetNewThrottleValue(CPU_INT32U throttle_value){
    g_current_throttle_value = throttle_value;
}

OS_SEM* GetNewThrottleEventSem()
{
  return &g_sem_new_throttle_event;
}

CPU_VOID configure_pwm(CPU_VOID){
    
    pwm_set_interrupt_mode(0);

    pwm_write_compare_2(DMA_CH2_CMP_VALUE);
    
   //init_pwm(); //sometime this should be uncommented compiled and flash so that the board works
    
    //dma enables the pwm channel
    //only cmp channel 2 changes here (ch1 with 100 remain)
    //no period changed at run 
}

CPU_INT08U configure_dma(CPU_VOID){
    
    //value to load in the pwm control register to start dma
    g_pwm_enable_value = PWM_1_CONTROL |= PWM_1_CTRL_ENABLE;
    
    //value to load in the pwm control register to stop dma
    g_pwm_disable_value = PWM_1_CONTROL &= ((uint8)(~PWM_1_CTRL_ENABLE));
    
    //initialize dshot frame buffer with 0
    update_dshot_frame_buffer(0);

    //This is the dma configuration that's done once
    
    //1 byte per burst, a request is needed for every burst exectuion, high 16 bits of source and destination address
    CPU_INT08U dma_channel = init_dma(ONE_BYTE, ENABLE_REQ_PER_BURST, HI16(g_cmp_values), HI16(PWM_1_COMPARE1_LSB_PTR));
    
    for(CPU_INT08U i = 0; i < DMA_AMOUNT_TDS; i++){
        g_td[i] = dma_td_allocate();
    }
    
    //confgure TDS and set theire source and destination addresses
    
    //setup first td to load first compare value. Next td is executed automatically
    dma_td_set_configuration(g_td[DMA_FIRST_CMP_VAL_TD], ONE_BYTE, g_td[DMA_START_TD], CY_DMA_TD_AUTO_EXEC_NEXT); //1 byte in total
    dma_td_set_address(g_td[DMA_FIRST_CMP_VAL_TD], LO16((CPU_INT32U)&g_cmp_values[0]), LO16((CPU_INT32U)PWM_1_COMPARE1_LSB_PTR));
    
    //enable PWM output by starting the DMA. Don't execute the next TD anymore (they will execute on the rising edge of pwm channel 2 from now on)
    dma_td_set_configuration(g_td[DMA_START_TD], ONE_BYTE, g_td[DMA_CMP_VALUES_TD], DMA_TD_NO_CONFIGURATION); //1 byte in total
    dma_td_set_address(g_td[DMA_START_TD], LO16((CPU_INT32U)&g_pwm_enable_value), LO16((CPU_INT32U)PWM_1_CONTROL_PTR));  
    
    //setup rest of cmp values. DSHOT_CMP_VALUES_LEN - 1 because the first one is set in the first td. Every td increases the src addr by 1 byte after executing
    dma_td_set_configuration(g_td[DMA_CMP_VALUES_TD], DSHOT_CMP_VALUES_LEN - 1, g_td[DMA_STOP_TD], CY_DMA_TD_INC_SRC_ADR); //DSHOT_CMP_VALUES_LEN - 1 bytes in total
    dma_td_set_address(g_td[DMA_CMP_VALUES_TD], LO16((CPU_INT32U)&g_cmp_values[1]), LO16((CPU_INT32U)PWM_1_COMPARE1_LSB_PTR));    
    
    //disable PWM output. Last td. Once this td executes the dma is disabled.
    dma_td_set_configuration(g_td[DMA_STOP_TD], ONE_BYTE, CY_DMA_DISABLE_TD, DMA_TD_NO_CONFIGURATION); //1 byte in total
    dma_td_set_address(g_td[DMA_STOP_TD], LO16((CPU_INT32U)&g_pwm_disable_value), LO16((CPU_INT32U)PWM_1_CONTROL_PTR));     
    
    return dma_channel;
}

CPU_VOID start_dma(CPU_INT08U dma_channel){

    //set the initial transfer descriptor to run
    dma_ch_set_init_td(dma_channel, g_td[DMA_FIRST_CMP_VAL_TD]);
    
    //enables dma channel and preserve the configuration of all tds once the td chain is finished
    dma_ch_enable(dma_channel, PRESERVE_TD_CHAIN_CONFIG); 
    
    //start the dma chain
    dma_ch_set_request(dma_channel, CY_DMA_CPU_REQ);
    
    CPU_INT08U state = 0;
    
    //wait until the dma chain execution is finished (to send the dshot frame pulse)
    do{
        dma_ch_status(dma_channel, NULL, &state);
    }while(state & CY_DMA_STATUS_CHAIN_ACTIVE);
}

CPU_INT16U get_dshot_frame_format(uint32_t throttle_value){
    
    // build 12-bit value: [11:1]=throttle, [0]=telemetry (0)
    CPU_INT16U dshot_frame = (throttle_value & 0x07FFu) << 1;   // telemetry = 0
    
    // CRC: ~(value ^ (value >> 4) ^ (value >> 8)) & 0x0F
    CPU_INT08U crc = (dshot_frame ^ (dshot_frame >> 4) ^ (dshot_frame >> 8)) & 0x0Fu;
    
    // final 16-bit frame: [15:4]=value, [3:0]=crc
    dshot_frame = (dshot_frame << 4) | crc;
    
    return dshot_frame;
}

CPU_VOID update_dshot_frame_buffer(uint32_t throttle_value){
    
    CPU_INT16U dshot_frame = get_dshot_frame_format(throttle_value);
    
    //actual dshot values go from [1:16]. Index 0 is a fill value.
    for(CPU_INT08U i = 1; i < DSHOT_CMP_VALUES_LEN; i++){
        
        // extract bit from MSB to LSB
        CPU_INT16U bit = (dshot_frame >> (15 - i)) & 0x01;
        
        // store PWM compare tick
        g_cmp_values[i] = bit ? DSHOT_BIT_1 : DSHOT_BIT_0;
    }
    
    //the fill value corresponds to the first dshot value
    g_cmp_values[0] = g_cmp_values[1];
}

/* [] END OF FILE */
