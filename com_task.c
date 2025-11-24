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

#include <dshot_task.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define MAX_INPUT_STRING_LENGTH  15u
#define MAX_OUTPUT_STRING_LENGTH 48u

#define  MSG_ERR_FORMAT         "ERROR: INPUT-> #rad value$\n"
#define  MSG_ERR_WRONG_CMD      "ERROR: CMD -> rad\n"
#define  MSG_ERR_LEADING_ZEROES "ERROR: leading zeroes\n"
#define  MSG_ERR_NO_DIGITS      "ERROR: value must be a int digit\n"
#define  MSG_ERR_VALUE_RANGE    "ERROR: value 0 or [48...1000]\n"


#define  MSG_MOTOR_RUN   "Rotating motor...\n"
#define  MSG_MOTOR_STOP  "Stoppping motor...\n"

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/
static OS_TCB App_TaskCom_TCB;
static CPU_STK App_TaskComStk_R[APP_CFG_TASK_COM_STK_SIZE];

CPU_INT32U g_user_rad_val;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                ResetCOMState()
*
* Description : This function resets the COM task state variables.
*
* Argument(s) : rx_msg        is the receive message buffer.
*               msg_size      is the size of the receive message buffer.
*               str_available is the string available flag.
*               rec_byte      is the received byte variable.
*               idx           is the index variable for the receive buffer.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskCOM().
*
* Note(s)     : none.
*********************************************************************************************************
*/
void ResetCOMState(CPU_INT08U *rx_msg, CPU_INT32U msg_size, CPU_BOOLEAN *str_available, CPU_INT08U *rec_byte, CPU_INT08U *idx);

CPU_BOOLEAN CheckInputCommand(CPU_INT08U* input_msg, CPU_INT08U* output_msg, CPU_INT32U* rad_value);
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

void App_TaskCom(void *p_arg){
    
  /* declare and define task local variables */
  OS_ERR os_err;
  CPU_INT08U rec_byte = 0x00;
  CPU_INT08U rx_msg[UART_1_RX_BUFFER_SIZE] = {0};
  CPU_INT08U idx = 0x00;
  CPU_INT08U rec_byte_cnt = 0x00;
  CPU_BOOLEAN str_available = DEF_FALSE;

  CPU_INT08U out_msg[MAX_OUTPUT_STRING_LENGTH];

  /* prevent compiler warnings */
  (void)p_arg;
  (void)Start_of_Packet;
  (void)End_of_Packet;

  /* start of the endless loop */
  while (DEF_TRUE){
    /* check if a byte is available */
    rec_byte = uart_get_byte();
    /* check if the received byte is '#'*/
    if (rec_byte == Start_of_Packet && DEF_FALSE == str_available){
      /* if received byte was correct */
      while (DEF_TRUE){
        /* receive byte by byte */
        rec_byte = uart_get_byte();
        /* check is byte is something meaningful */
        if (rec_byte){
          /* save byte into software receive buffer and increment idx */
          rx_msg[idx++] = rec_byte;
        }
        /* initiate scheduler */
        OSTimeDlyHMSM(0, 0, 0, 20, OS_OPT_TIME_HMSM_STRICT, &os_err);

        /* check if received byte is '$' */
        if (rx_msg[idx - 1] == End_of_Packet){
          /* if end of packet is reached -> break */
          break;
        }
      }

      rx_msg[idx - 1] = '\0'; // terminate the input string

      /* message received, calculate received bytes, -2 because of '#' & '$' */
      rec_byte_cnt = idx - 2;

      // do not accept more bytes than expected
      if (rec_byte_cnt > MAX_INPUT_STRING_LENGTH){
        ResetCOMState(&rx_msg[0], sizeof(rx_msg), &str_available, &rec_byte, &idx);
        
      }else{
        /* signal that a string is available */
        str_available = DEF_TRUE;
      }
    }
     /* if received byte wasn't start of packet */
    else if (DEF_FALSE == str_available){
      ResetCOMState(&rx_msg[0], sizeof(rx_msg), &str_available, &rec_byte, &idx);
    }
    
    /* check if message is available */
    if (str_available){
        // check input command for validity
        CPU_BOOLEAN is_error = CheckInputCommand(rx_msg, out_msg, &g_user_rad_val);

        if (!is_error){
            // Check valid range MOTOR_THROTTLE_OFF or MOTOR_THROTTLE_MIN up to MOTOR_THROTTLE_MAX
            if ((g_user_rad_val > MOTOR_THROTTLE_OFF && g_user_rad_val < MOTOR_THROTTLE_MIN) || g_user_rad_val > MOTOR_THROTTLE_MAX){
              strncpy((CPU_CHAR*)out_msg, MSG_ERR_VALUE_RANGE, strlen(MSG_ERR_VALUE_RANGE) + 1);  
              is_error = DEF_TRUE;
            
            }else{
              // post message to dshot task. Validity of value already checked
              OSTaskQPost(GetDshotTaskTCB(), &g_user_rad_val, sizeof(g_user_rad_val), OS_OPT_POST_FIFO, &os_err);

              //message sent, notify user
              if(OS_ERR_NONE == os_err){
                if(0 == g_user_rad_val){
                    strncpy((CPU_CHAR*)out_msg, MSG_MOTOR_STOP, strlen(MSG_MOTOR_STOP) + 1);
                }else{
                    strncpy((CPU_CHAR*)out_msg, MSG_MOTOR_RUN, strlen(MSG_MOTOR_RUN) + 1);
                }
              }
            }
        }
        
        //notify user on what happened
        const CPU_INT32U real_msg_length = strlen((CPU_CHAR*)out_msg);

        // send output message via UART byte by byte
        for (CPU_INT08U jdx = 0x00; jdx < real_msg_length; jdx++){
          uart_send_byte(out_msg[jdx]);
        }
        
        ResetCOMState(&rx_msg[0], sizeof(rx_msg), &str_available, &rec_byte, &idx);
    }
    
    /* initiate scheduler */
    OSTimeDlyHMSM(0, 0, 0, COM_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
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

/*
*********************************************************************************************************
*                                                ResetCOMState()
*
* Description : This function resets the COM task state variables.
*
* Argument(s) : rx_msg        is the receive message buffer.
*               msg_size      is the size of the receive message buffer.
*               str_available is the string available flag.
*               rec_byte      is the received byte variable.
*               idx           is the index variable for the receive buffer.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskCOM().
*
* Note(s)     : none.
*********************************************************************************************************
*/
void ResetCOMState(CPU_INT08U *rx_msg, CPU_INT32U msg_size, CPU_BOOLEAN *str_available, CPU_INT08U *rec_byte, CPU_INT08U *idx)
{
  /* reset software receive buffer */
  memset(rx_msg, 0, msg_size);
  /* reset string available signal */
  *str_available = DEF_FALSE;
  /* reset received byte variable */
  *rec_byte = 0x00;
  /* reset idx */
  *idx = 0x00;
}


CPU_BOOLEAN CheckInputCommand(CPU_INT08U* input_msg, CPU_INT08U* output_msg, CPU_INT32U* rad_value){
    
  CPU_BOOLEAN is_error = DEF_FALSE;
  CPU_CHAR *token = NULL;
  CPU_INT32U token_nr = 0;

  CPU_CHAR* command = NULL;
  CPU_CHAR* value = NULL;
    
   // empty output message
   memset(output_msg, 0, MAX_OUTPUT_STRING_LENGTH);

  // tokenize input string. Split by space " "
  token = strtok((CPU_CHAR*)input_msg, " ");

  while (NULL != token){
    if (0 == token_nr){
      command = token; // first token is command
    }else if (1 == token_nr){
      value = token; // second token is value
    }else{
      // Only 2 tokens allowed as input. More is not allowed
      is_error = DEF_TRUE;
      break;
    }

    token = strtok(NULL, " ");
    token_nr++;
  }

  // Check for missing command or value
  if (NULL == command || NULL == value){
    // format output message for error
    strncpy((CPU_CHAR*)output_msg, MSG_ERR_FORMAT, strlen(MSG_ERR_FORMAT) + 1);
    is_error = DEF_TRUE;
  }

  // tokens extracted, now check validity
  if (!is_error){
    // command must be either "sin" or "cos"
    if (0 != strcmp(command, "rad")){
      strncpy((CPU_CHAR*)output_msg, MSG_ERR_WRONG_CMD, strlen(MSG_ERR_WRONG_CMD) + 1);
      is_error = DEF_TRUE;
    }

      // Check for leading zero. E.g. 007
      if (value[0] == '0' && value[1] != '\0'){
        strncpy((CPU_CHAR*)output_msg, MSG_ERR_LEADING_ZEROES, strlen(MSG_ERR_LEADING_ZEROES) + 1);
        is_error = DEF_TRUE;
      }

      // Check that all characters are digits
      for (const CPU_CHAR *character = value; *character != '\0'; character++){
        if (!isdigit((CPU_CHAR)*character)){
          strncpy((CPU_CHAR*)output_msg, MSG_ERR_NO_DIGITS, strlen(MSG_ERR_NO_DIGITS) + 1);
          is_error = DEF_TRUE;
          break;
        }
      }
    
     if(!is_error){
       *rad_value = atoi(value);
     }
  }

  return is_error;
}

/* [] END OF FILE */
