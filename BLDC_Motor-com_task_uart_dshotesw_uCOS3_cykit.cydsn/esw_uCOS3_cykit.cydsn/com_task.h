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
 
/* Queue for sending throttle commands from Com Task to DShot Task
*/
OS_Q g_com_to_dshot_queue;
  
/*max payload size between # and $ */
#define COM_MAX_PAYLOAD 8u
/*Number (quantity) of command buffers-
  How many command blocks are in the COM memory partition*/
#define COM_CMD_QTY 4u
  
/*command structure sent to DShot task */
typedef struct{
  CPU_INT08U payload[COM_MAX_PAYLOAD];
  CPU_INT08U length;
}com_cmd_t;

void ComTask_Init(void);
OS_Q* ComTask_GetQueue(void);
OS_MEM* ComTask_GetMem(void);   /* For DShot to free blocks */

void SendThrottleToDShot(CPU_INT16U throttle);
  
/*
*********************************************************************************************************
*                                          App_TaskCom()
*
* Description : COM Task checks for available bytes within the UART receive buffer. If correct string is
*               available (e.g. PC -> uC: #abc$ or #Hellor World$), process the message and output a
*               pre-defined string via UART and append the user-defined string via the UART interface.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskCom()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
void App_TaskCom(void *p_arg);

/*
*********************************************************************************************************
*                                          GetComTaskTCB()
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
OS_TCB* GetComTaskTCB();

/*
*********************************************************************************************************
*                                          GetComTaskStk()
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
CPU_STK* GetComTaskStk();

#endif
/* [] END OF FILE */
