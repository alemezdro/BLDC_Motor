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
#include <com_task.h>
#include <includes.h>
#include <bsp_uart.h>


/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define COM_RX_BUF_SIZE  16u

#define PARSE_TOO_BIG ((CPU_INT16U)65535u)  //>2047
#define PARSE_INVALID ((CPU_INT16U)65534u)  // letters, no digits

#define THROTTLE_MIN 48u
#define THROTTLE_MAX 2047u


/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/
static OS_TCB App_TaskCom_TCB;
static CPU_STK App_TaskComStk_R[APP_CFG_TASK_COM_STK_SIZE];

/* communication with DShot task */
static CPU_INT08U mem_pool[COM_CMD_QTY][sizeof(com_cmd_t)];
static OS_MEM mem_part;
/* message queue to pass pointers */
static OS_Q queue_to_dshot;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                     static CPU_INT16U ParseNumber()
*
* Description : Converts an ASCII decimal string (0..2047)
*               to a 16-bit unsigned integer.
*               Ignores ' ', '\t', '\r', '\n', and '$'.
*               Any other non-digit character is invalid.
* Argument(s) : buf - Pointer to input buffer
*               len - Number of bytes to scan
*
* Return(s)   : Parsed value (0..2047) on success
*               PARSE_TOO_BIG  - Value exceeds THROTTLE_MAX
*               PARSE_INVALID  - No digits or illegal characters found
*
* Note(s)     : none
*********************************************************************************************************
*/

static CPU_INT16U ParseNumber(const CPU_INT08U *buf,  CPU_INT08U len){
  
  CPU_INT16U value = 0u;
  CPU_INT08U i;
  CPU_BOOLEAN has_digit = DEF_FALSE;
  
  for (i = 0u; i < len; i++) {
    CPU_INT08U ch = buf[i];
    if (ch >= '0' && ch <= '9') {
      has_digit = DEF_TRUE;
      value = value * 10u + (ch - '0');
      if (value > THROTTLE_MAX) {
        return PARSE_TOO_BIG; // overflow
        // value = THROTTLE_MAX;
      }
    }
      else if (ch != '\r' && ch != '\n' && ch != '$' && ch != ' ' && ch != '\t')
        {
            return PARSE_INVALID;              
        }
    }
  if (!has_digit){
    return PARSE_INVALID;
  }
    return (CPU_INT16U)value;       
}
 


/*
*********************************************************************************************************
*                                          ComTask_Init()
*
* Description : Function that initialize partition and creates queue (called once from app.c)
*               Create partition: COM_CMD_BLOCK_QTY blocks of size com_cmd_t
* Argument(s) : 
*
* Return(s)   : 0
*
* Note(s)     : none
*********************************************************************************************************
*/
void ComTask_Init(void){
  OS_ERR  err;
 
  OSMemCreate(&mem_part, 
              "COM Cmd Mem", 
              mem_pool, 
              COM_CMD_QTY, 
              sizeof(com_cmd_t),
              &err);
 
  OSQCreate(&queue_to_dshot, "Q COM to DShot", COM_CMD_QTY, &err);
  (void)err;
}

/*
*********************************************************************************************************
*                                          ComTask_GetQueue()
*
* Description : Get Queue for Dschot task
*
* Argument(s) : none
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/

OS_Q* ComTask_GetQueue(void){
  return &queue_to_dshot;
}


/*
*********************************************************************************************************
*                                          ComTask_GetMem()
*
* Description : Get memory for Dschot task
*
* Argument(s) : none
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
OS_MEM* ComTask_GetMem(void){
  return &mem_part;
}


/*
*********************************************************************************************************
*                                          ComTask_SendToDShot()
*
* Description : Sends a 16-bit DShot command value (0..2047) to the DShot task via message queue.
*               Allocates a command block from the memory partition,
*               fills in the 16-bit value(little-endian: low byte first),
*               sets payload length = 2, and posts it to the COM → DShot queue.
*               This function is called from the COM task when
*               a valid command like #1500$ or #RPM:1000$ is received from the PC.
*
* Argument(s) : value: DShot throttle/command value (0 = disarm, 48 = arm,
*               1..2047 = throttle,
*               special values like 21 = reverse beep, etc.)
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/

void SendThrottleToDShot(CPU_INT16U throttle){
  OS_ERR  err;
  com_cmd_t *p_cmd;
  
  p_cmd = (com_cmd_t *)OSMemGet(&mem_part, &err);
  if (p_cmd == NULL){
    return;
  }
    p_cmd->payload[0] = (CPU_INT08U)(throttle & 0xFFu); 
    p_cmd->payload[1] = (CPU_INT08U)(throttle >> 8u);
    p_cmd->length = 2u;
 
  OSQPost(ComTask_GetQueue(), p_cmd, sizeof(void*),OS_OPT_POST_FIFO, &err);
  (void)err; 
}


/*
*********************************************************************************************************
*                                          App_TaskCom()
*
* Description : Communication task that receives ASCII commands over UART.
*               Waits for packets starting with '#' and 
*               ending with '$', '\r' or '\n'.
*               Extracts the numeric value after '#', validates it (48..2047),
*               and sends the corresponding throttle via DShot if valid.
*               Replies with "CY8KIT: OK: <value>" on success 
*               or an error message otherwise.
* Argument(s) : p_arg  argument passed  by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : - Packet format: "#<number>$" (or terminated by \r/\n)
*               - Uses non-blocking uart_get_byte() → processes one byte per tick
*               - 1 ms delay each loop iteration
*********************************************************************************************************
*/

void  App_TaskCom (void *p_arg)
{
  /* declare and define task local variables */
  OS_ERR       os_err;
  CPU_INT08U   ch;
  static CPU_INT08U   rx_buf[COM_RX_BUF_SIZE];
  CPU_INT08U   idx = 0u;
  CPU_BOOLEAN  packet_ready = DEF_FALSE;
  
  /* prevent compiler warnings */
  (void)p_arg;
 // (void)Start_of_Packet;
  //(void)End_of_Packet;
  
  init_uart();
  
  /* start of the endless loop */
  while (DEF_TRUE) {
    /* check if a byte is available */
    ch = uart_get_byte();

        if (ch != 0){  // valid byte received
          if (ch == '#'){
            idx = 0;                    // start of new packet
            rx_buf[idx++] = ch;
          }
          else if (idx > 0 && idx < sizeof(rx_buf)-1){
            rx_buf[idx++] = ch;
            if (ch == '$' || ch == '\n' || ch == '\r'){
              rx_buf[idx-1] = '\0';  // null-terminate before $
              packet_ready = DEF_TRUE;
            }
          }
        }

        /* Process complete packet*/
       if (packet_ready){
        packet_ready = DEF_FALSE;
        idx = 0;
        
        /* Find the part after '#' */
        char *payload = strchr((char*)rx_buf, '#');
        if (payload) payload++;           // skip the '#'
        else payload = (char*)rx_buf;     // safety

        /* Parse a number from payload */
        CPU_INT16U value = ParseNumber((CPU_INT08U*)payload, strlen(payload));
        UART_1_PutString("CY8KIT: ");

        /* Valid range */
        if (value == 0u || (value >= THROTTLE_MIN && value <= THROTTLE_MAX)){
          SendThrottleToDShot(value);
          
          UART_1_PutString("OK: ");
          UART_1_PutString(payload);
          UART_1_PutString("\r\n");
        }
        /* Out of range */
        else{
          UART_1_PutString("input error: ");
          UART_1_PutString(payload);
              
          if(value == PARSE_INVALID){
          UART_1_PutString(" (invalid)");
          }
          else if(value == PARSE_TOO_BIG || value > THROTTLE_MAX){
          UART_1_PutString(" (max 2047)");
          }
          else if (value > 0u &&value < THROTTLE_MIN){
          UART_1_PutString(" (min 48)");
          } 
          UART_1_PutString("\r\n");
        }
      }
      OSTimeDly(1, OS_OPT_TIME_DLY, &os_err);
  }
}

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
OS_TCB* GetComTaskTCB()
{
  return &App_TaskCom_TCB;
}

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
CPU_STK* GetComTaskStk()
{
  return &App_TaskComStk_R[0];
}

/* [] END OF FILE */
