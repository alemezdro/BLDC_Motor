/*
*********************************************************************************************************
*
*                                       MES1 Embedded Software (RTOS)
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : Beneder Roman

*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <includes.h>
#include <cyapicallbacks.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/

#define APP_USER_IF_SIGN_ON 0u
#define APP_USER_IF_VER_TICK_RATE 1u
#define APP_USER_IF_CPU 2u
#define APP_USER_IF_CTXSW 3u
#define APP_USER_IF_STATE_MAX 4u

#define USE_BSP_TOGGLE 1u

#define GENERAL_TASK_DELAY_MS 100u

#define TRIGONOMETRY_MAX_VALUES 361u

#define MAX_AMOUNT_INPUT_STRINGS 2u
#define MAX_INPUT_STRING_LENGTH 8u

#define MAX_OUTPUT_STRING_LENGTH 20u

#define COM_TASK_CMD_RX_TIMEOUT_MS 200u

#define MESSAGE_QUEUE_LENGTH 2u

#define TASK_QUEUE_LENGTH 1u // except for CMD task

#define DEG2RAD(x) ((x) * (float)M_PI / 180.0f)
#define EPS 1e-5f

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/
static OS_TCB App_TaskStartTCB;
static CPU_STK App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static OS_TCB App_TaskCMD_TCB;
static CPU_STK App_TaskCMDStk_R[APP_CFG_TASK_CMD_STK_SIZE];

static OS_TCB App_TaskSIN_TCB;
static CPU_STK App_TaskSINStk_G[APP_CFG_TASK_SIN_STK_SIZE];

static OS_TCB App_TaskCOS_TCB;
static CPU_STK App_TaskCOSStk_B[APP_CFG_TASK_COS_STK_SIZE];

static OS_TCB App_TaskCOM_TCB;
static CPU_STK App_TaskCOMStk[APP_CFG_TASK_COM_STK_SIZE];

// Lookup tables for sin and cos values
static CPU_FP32 g_out_sin_values[TRIGONOMETRY_MAX_VALUES];
static CPU_FP32 g_out_cos_values[TRIGONOMETRY_MAX_VALUES];

// Sin and Cos message queues for communication (SIN,COS) Task -> Task CMD
// No semaphore needed for these because values are read only!
static OS_Q g_msg_queue_sin_values;
static OS_Q g_msg_queue_cos_values;

// input message buffers
static CPU_CHAR g_in_msgs[MAX_AMOUNT_INPUT_STRINGS][MAX_INPUT_STRING_LENGTH];

// output message buffer
static CPU_CHAR g_out_msg[MAX_OUTPUT_STRING_LENGTH];

// variables to hold input degree values for sin and cos
static CPU_INT32U g_in_sin_value;
static CPU_INT32U g_in_cos_value;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void App_TaskStart(void *p_arg);
static void App_TaskCreate(void);
static void App_ObjCreate(void);

static void App_TaskCMD(void *p_arg);
static void App_TaskSIN(void *p_arg);
static void App_TaskCOS(void *p_arg);
static void App_TaskCOM(void *p_arg);

static void InitSinTable();
static void InitCosTable();

/*
*********************************************************************************************************
*                                                MapFloatToString()
*
* Description : This function maps a float value to a string with format "-x.xx" or "x.xx".
*
* Argument(s) : buff      is the pointer to the buffer where the string will be stored.
*               buff_len  is the length of the buffer.
*               value     is the float value to be converted to string.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSIN(), App_TaskCOS().
*
* Note(s)     : NULL is returned if the buffer length is insufficient otherwise the pointer to the buffer is returned.
*********************************************************************************************************
*/
static CPU_CHAR *MapFloatToString(CPU_CHAR *buff, CPU_INT32U buff_len, CPU_FP32 value);

/*
*********************************************************************************************************
*                                                CheckInputCommand()
*
* Description : This function checks the input command string for validity.
*
* Argument(s) : string    is the input command string.
*               command   is the pointer to the command part of the input string.
*               value     is the pointer to the value part of the input string.
*
* Return(s)   : CPU_BOOLEAN DEF_TRUE if error, DEF_FALSE if no error.
*
* Caller(s)   : App_TaskCMD().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static CPU_BOOLEAN CheckInputCommand(CPU_CHAR *string, CPU_CHAR **command, CPU_CHAR **value);

/*
*********************************************************************************************************
*                                                CmdProcessDegreeValue()
*
* Description : This function processes the degree value for the sine or cosine calculation. It sends the
*               degree value to the respective task and waits for the result. Once the result is received,
*               it formats the output message and sends it to the COM task.
*
* Argument(s) : degree_val  is the degree value to be processed.
*               for_sinus   is a boolean indicating whether the value is for sine (DEF_TRUE) or cosine (DEF_FALSE).
*
* Return(s)   : none
*
* Caller(s)   : App_TaskCMD().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static void CmdProcessDegreeValue(CPU_INT32U degree_val, CPU_BOOLEAN for_sinus);

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

/*
*********************************************************************************************************
*                                                InitSinTable()
*
* Description : This function initializes the sine lookup table.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : main().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static void InitSinTable()
{

  for (CPU_INT32U idx = 0; idx < TRIGONOMETRY_MAX_VALUES; idx++)
  {
    CPU_FP32 value = sinf(DEG2RAD((float)idx));

    if (fabsf(value) < EPS)
    {
      value = 0.0f;
    }

    g_out_sin_values[idx] = value;
  }
}

/*
*********************************************************************************************************
*                                                InitCosTable()
*
* Description : This function initializes the cosine lookup table.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : main().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static void InitCosTable()
{
  for (CPU_INT32U idx = 0; idx < TRIGONOMETRY_MAX_VALUES; idx++)
  {

    CPU_FP32 value = cosf(DEG2RAD((float)idx));

    if (fabsf(value) < EPS)
    {
      value = 0.0f;
    }

    g_out_cos_values[idx] = value;
  }
}

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization. In this functions, the sin
*               and cos lookup tables are initialized and the start task is created.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : Startup Code.
*
* Note(s)     : none.
*********************************************************************************************************
*/

int main(void)
{
  OS_ERR os_err;

  BSP_PreInit(); /* Perform BSP pre-initialization.                      */

  CPU_Init(); /* Initialize the uC/CPU services                       */

  OSInit(&os_err); /* Init uC/OS-III.                                      */

  InitSinTable(); /* Initialize the sine lookup table.                   */

  InitCosTable(); /* Initialize the cosine lookup table.                 */

  OSTaskCreate((OS_TCB *)&App_TaskStartTCB, /* Create the start task                                */
               (CPU_CHAR *)"Start",
               (OS_TASK_PTR)App_TaskStart,
               (void *)0,
               (OS_PRIO)APP_CFG_TASK_START_PRIO,
               (CPU_STK *)&App_TaskStartStk[0],
               (CPU_STK_SIZE)APP_CFG_TASK_START_STK_SIZE_LIMIT,
               (CPU_STK_SIZE)APP_CFG_TASK_START_STK_SIZE,
               (OS_MSG_QTY)0u,
               (OS_TICK)0u,
               (void *)0,
               (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR *)&os_err);

  OSStart(&os_err); /* Start multitasking (i.e. give control to uC/OS-III).  */
}

/*
*********************************************************************************************************
*                                          App_TaskStart()
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static void App_TaskStart(void *p_arg)
{
  OS_ERR err;

  (void)p_arg;

  BSP_PostInit(); /* Perform BSP post-initialization functions.       */

  BSP_CPU_TickInit(); /* Perfrom Tick Initialization                      */

#if (OS_CFG_STAT_TASK_EN > 0u)
  OSStatTaskCPUUsageInit(&err);
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  CPU_IntDisMeasMaxCurReset();
#endif

  App_TaskCreate(); /* Create application tasks.                        */

  App_ObjCreate(); /* Create kernel objects                            */

  while (DEF_TRUE)
  { /* Task body, always written as an infinite loop.   */
    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS,
                  OS_OPT_TIME_HMSM_STRICT,
                  &err);
  }
}

/*
*********************************************************************************************************
*                                          App_TaskCreate()
*
* Description : Create application tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static void App_TaskCreate(void)
{
  /* declare and define function local variables */
  OS_ERR os_err;

  /* create CMD Task channel*/
  OSTaskCreate((OS_TCB *)&App_TaskCMD_TCB,
               (CPU_CHAR *)"TaskCMD",
               (OS_TASK_PTR)App_TaskCMD,
               (void *)0,
               (OS_PRIO)APP_CFG_TASK_CMD_PRIO,
               (CPU_STK *)&App_TaskCMDStk_R[0],
               (CPU_STK_SIZE)APP_CFG_TASK_CMD_STK_SIZE_LIMIT,
               (CPU_STK_SIZE)APP_CFG_TASK_CMD_STK_SIZE,
               (OS_MSG_QTY)MAX_AMOUNT_INPUT_STRINGS,
               (OS_TICK)0u,
               (void *)0,
               (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR *)&os_err);
  /* create SIN Task channel*/
  OSTaskCreate((OS_TCB *)&App_TaskSIN_TCB,
               (CPU_CHAR *)"TaskSINUS",
               (OS_TASK_PTR)App_TaskSIN,
               (void *)0,
               (OS_PRIO)APP_CFG_TASK_SIN_PRIO,
               (CPU_STK *)&App_TaskSINStk_G[0],
               (CPU_STK_SIZE)APP_CFG_TASK_SIN_STK_SIZE_LIMIT,
               (CPU_STK_SIZE)APP_CFG_TASK_SIN_STK_SIZE,
               (OS_MSG_QTY)TASK_QUEUE_LENGTH,
               (OS_TICK)0u,
               (void *)0,
               (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR *)&os_err);
  /* create COS Task channel*/
  OSTaskCreate((OS_TCB *)&App_TaskCOS_TCB,
               (CPU_CHAR *)"TaskCOSINUS",
               (OS_TASK_PTR)App_TaskCOS,
               (void *)0,
               (OS_PRIO)APP_CFG_TASK_COS_PRIO,
               (CPU_STK *)&App_TaskCOSStk_B[0],
               (CPU_STK_SIZE)APP_CFG_TASK_COS_STK_SIZE_LIMIT,
               (CPU_STK_SIZE)APP_CFG_TASK_COS_STK_SIZE,
               (OS_MSG_QTY)TASK_QUEUE_LENGTH,
               (OS_TICK)0u,
               (void *)0,
               (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR *)&os_err);
  /* create COM task */
  OSTaskCreate((OS_TCB *)&App_TaskCOM_TCB,
               (CPU_CHAR *)"TaskCOM",
               (OS_TASK_PTR)App_TaskCOM,
               (void *)0,
               (OS_PRIO)APP_CFG_TASK_COM_PRIO,
               (CPU_STK *)&App_TaskCOMStk[0],
               (CPU_STK_SIZE)APP_CFG_TASK_COM_STK_SIZE_LIMIT,
               (CPU_STK_SIZE)APP_CFG_TASK_COM_STK_SIZE,
               (OS_MSG_QTY)TASK_QUEUE_LENGTH,
               (OS_TICK)0u,
               (void *)0,
               (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR *)&os_err);
}

/*
*********************************************************************************************************
*                                          App_ObjCreate()
*
* Description : Create application kernel objects tasks.
*
* Argument(s) : none
*
* Return(s)   : none
*
* Caller(s)   : AppTaskStart()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static void App_ObjCreate(void)
{
  /* declare and define function local variables */
  OS_ERR os_err;

  /* create message queues for sin and cos values*/
  OSQCreate(&g_msg_queue_sin_values, "MSG_QUEUE_SIN_VAL", MESSAGE_QUEUE_LENGTH, &os_err);
  OSQCreate(&g_msg_queue_cos_values, "MSG_QUEUE_COS_VAL", MESSAGE_QUEUE_LENGTH, &os_err);
}

/*
*********************************************************************************************************
*                                                MapFloatToString()
*
* Description : This function maps a float value to a string with format "-x.xx" or "x.xx".
*
* Argument(s) : buff      is the pointer to the buffer where the string will be stored.
*               buff_len  is the length of the buffer.
*               value     is the float value to be converted to string.
*
* Return(s)   : none
*
* Caller(s)   : App_TaskSIN(), App_TaskCOS().
*
* Note(s)     : NULL is returned if the buffer length is insufficient otherwise the pointer to the buffer is returned.
*********************************************************************************************************
*/
static CPU_CHAR *MapFloatToString(CPU_CHAR *buff, CPU_INT32U buff_len, CPU_FP32 value)
{

  // format "-x.xx" or "x.xx"
  const CPU_INT32U min_length = 6;
  CPU_CHAR *res = NULL;

  if (buff_len >= min_length)
  {

    res = buff;

    // Check for negative values
    if (signbit(value))
    {
      buff[0] = '-';
      value *= -1.0f; // make positive
    }
    else
    {
      buff[0] = ' ';
    }

    CPU_INT32U int_val = value; // either 1 o 0

    buff[1] = '0' + int_val; // integer part
    buff[2] = '.';

    int_val = value * 100.0f; // only 2 decimal values

    buff[3] = '0' + ((int_val / 10) % 10); // first decimal
    buff[4] = '0' + (int_val % 10);        // second decimal
    buff[5] = '\0';
  }

  return res;
}

/*
*********************************************************************************************************
*                                                CheckInputCommand()
*
* Description : This function checks the input command string for validity.
*
* Argument(s) : string    is the input command string.
*               command   is the pointer to the command part of the input string.
*               value     is the pointer to the value part of the input string.
*
* Return(s)   : CPU_BOOLEAN DEF_TRUE if error, DEF_FALSE if no error.
*
* Caller(s)   : App_TaskCMD().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static CPU_BOOLEAN CheckInputCommand(CPU_CHAR *string, CPU_CHAR **command, CPU_CHAR **value)
{

  CPU_BOOLEAN is_error = DEF_FALSE;
  CPU_CHAR *token = NULL;
  CPU_INT32U token_nr = 0;

  *command = NULL;
  *value = NULL;

  // tokenize input string. Split by space " "
  token = strtok(string, " ");

  while (NULL != token)
  {
    if (0 == token_nr)
    {
      *command = token; // first token is command
    }
    else if (1 == token_nr)
    {
      *value = token; // second token is value
    }
    else
    {
      // Only 2 tokens allowed as input. More is not allowed
      is_error = DEF_TRUE;
      break;
    }

    token = strtok(NULL, " ");
    token_nr++;
  }

  // Check for missing command or value
  if (NULL == *command || NULL == *value)
  {
    is_error = DEF_TRUE;
  }

  // tokens extracted, now check validity
  if (!is_error)
  {

    // command must be either "sin" or "cos"
    if (0 != strcmp(*command, "sin") && 0 != strcmp(*command, "cos"))
    {
      is_error = DEF_TRUE;
    }

    // if value not all, check that it is a number
    if (0 != strcmp(*value, "all"))
    {
      // Check for leading zero. E.g. 007
      if ((*value)[0] == '0' && (*value)[1] != '\0')
      {
        is_error = DEF_TRUE;
      }

      // Check that all characters are digits
      for (const CPU_CHAR *character = *value; *character != '\0'; character++)
      {
        if (!isdigit((CPU_CHAR)*character))
        {
          is_error = DEF_TRUE;
          break;
        }
      }
    }
  }

  return is_error;
}

/*
*********************************************************************************************************
*                                                CmdProcessDegreeValue()
*
* Description : This function processes the degree value for the sine or cosine calculation. It sends the
*               degree value to the respective task and waits for the result. Once the result is received,
*               it formats the output message and sends it to the COM task.
*
* Argument(s) : degree_val  is the degree value to be processed.
*               for_sinus   is a boolean indicating whether the value is for sine (DEF_TRUE) or cosine (DEF_FALSE).
*
* Return(s)   : none
*
* Caller(s)   : App_TaskCMD().
*
* Note(s)     : none.
*********************************************************************************************************
*/
static void CmdProcessDegreeValue(CPU_INT32U degree_val, CPU_BOOLEAN for_sinus)
{
  // Process degree value

  CPU_TS ts;
  OS_ERR os_err;
  OS_MSG_SIZE msg_size = 0;

  CPU_INT32U *var_to_send = NULL;
  OS_Q *msg_queue_to_use = NULL;
  OS_TCB *p_task_tcb = NULL;

  // determine which task to use based on for_sinus flag
  if (for_sinus)
  {
    var_to_send = &g_in_sin_value;
    msg_queue_to_use = &g_msg_queue_sin_values;
    p_task_tcb = &App_TaskSIN_TCB;
  }
  else
  {
    var_to_send = &g_in_cos_value;
    msg_queue_to_use = &g_msg_queue_cos_values;
    p_task_tcb = &App_TaskCOS_TCB;
  }

  // send degree value to respective task
  *var_to_send = degree_val;

  // post the degree value to the respective task
  OSTaskQPost(p_task_tcb, var_to_send, sizeof(*var_to_send), OS_OPT_POST_FIFO, &os_err);

  // wait for result from respective task (sin or cos). Pending indefinitely.
  CPU_FP32 *conversion = (CPU_FP32 *)OSQPend(msg_queue_to_use, 0, OS_OPT_PEND_BLOCKING, &msg_size, &ts, &os_err);

  if (NULL != conversion)
  {
    CPU_CHAR buff[6];

    // map float to string
    MapFloatToString(buff, sizeof(buff), *conversion);

    // prepare output message
    memset(g_out_msg, 0, MAX_OUTPUT_STRING_LENGTH);

    // format output message based on sine or cosine
    if (for_sinus)
    {
      snprintf(g_out_msg, sizeof(g_out_msg), "SINE-RES:%s\n", buff);
    }
    else
    {
      snprintf(g_out_msg, sizeof(g_out_msg), "COSINE-RES:%s\n", buff);
    }

    // post output message to COM task
    OSTaskQPost(&App_TaskCOM_TCB, &g_out_msg, sizeof(g_out_msg), OS_OPT_POST_FIFO, &os_err);
  }
}

/*
*********************************************************************************************************
*                                          App_TaskCMD()
*
* Description : This task handles command processing. It waits for input commands, validates them,
*               and processes the corresponding sine or cosine calculations.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskCMD()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
static void App_TaskCMD(void *p_arg)
{
  /* prevent compiler warnings */
  (void)p_arg;

  OS_MSG_SIZE com_msg_size = 0;
  CPU_TS ts;
  OS_ERR os_err;

  CPU_CHAR *input_command = NULL;
  CPU_CHAR *cmd = NULL;
  CPU_CHAR *degree = NULL;
  CPU_INT32S degree_val = 0; // if -1, all values in 10 degree step are required

  while (DEF_TRUE)
  {

    // wait for input command from COM task. Pending indefinitely.
    input_command = (CPU_CHAR *)OSTaskQPend(0, OS_OPT_PEND_BLOCKING, &com_msg_size, &ts, &os_err);

    // check input command for validity
    CPU_BOOLEAN is_error = CheckInputCommand(input_command, &cmd, &degree);

    if (!is_error)
    {

      // process degree value
      if (0 == strcmp(degree, "all"))
      {
        degree_val = -1; // indicate all values with value -1
      }
      else
      {
        // Convert string to integer
        degree_val = atoi(degree);

        // Check valid range 0-360
        if (degree_val > 360)
        {
          is_error = DEF_TRUE;
        }
      }
    }

    if (is_error)
    {
      // prepare error message
      memset(g_out_msg, 0, MAX_OUTPUT_STRING_LENGTH);

      // format output message for error
      strncpy(g_out_msg, "INPUT ERROR\n", strlen("INPUT ERROR\n") + 1);

      // post error message to COM task
      OSTaskQPost(&App_TaskCOM_TCB, &g_out_msg, sizeof(g_out_msg), OS_OPT_POST_FIFO, &os_err);
    }
    else
    {
      // cmd must be either "sin" or "cos" at this point
      CPU_BOOLEAN for_sinus = DEF_TRUE; // assume its for sinus

      // check if command is for cosine, if yes, set for_sinus to false
      if (0 == strcmp("cos", cmd))
      {
        for_sinus = DEF_FALSE;
      }

      if (-1 == degree_val)
      {
        // process all values in 10 degree steps (0,10,20,...,360)
        for (CPU_INT32U idx = 0; idx < TRIGONOMETRY_MAX_VALUES; idx += 10)
        {
          CmdProcessDegreeValue(idx, for_sinus);
        }
      }
      else
      {
        // process single degree value
        CmdProcessDegreeValue(degree_val, for_sinus);
      }
    }

    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
  }
}

/*
*********************************************************************************************************
*                                          App_TaskSIN()
*
* Description : This task calculates the sine value for a given degree input. It waits for degree values
*               from the CMD task, retrieves the corresponding sine value from the lookup table, and
*               sends the result back to the CMD task.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskSIN()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
static void App_TaskSIN(void *p_arg)
{

  /* prevent compiler warnings */
  (void)p_arg;

  OS_MSG_SIZE cmd_msg_size = 0;
  OS_ERR os_err;
  CPU_TS ts;
  CPU_INT32U degree_val = 0;

  while (DEF_TRUE)
  {
    // wait for degree value from CMD task. Pending indefinitely.
    CPU_INT32U *sin_degree = (CPU_INT32U *)OSTaskQPend(0, OS_OPT_PEND_BLOCKING, &cmd_msg_size, &ts, &os_err);

    if (NULL != sin_degree)
    {
      degree_val = *sin_degree; // get degree value if valid value extracted from task queue
    }
    else
    {
      degree_val = 0; // post sin(0) if error occurs
    }

    // post sine value back to CMD task.
    OSQPost(&g_msg_queue_sin_values, &g_out_sin_values[degree_val],
            sizeof(g_out_sin_values[degree_val]), OS_OPT_POST_FIFO, &os_err);

    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
  }
}

/*
*********************************************************************************************************
*                                          App_TaskCOS()
*
* Description : This task calculates the cosine value for a given degree input. It waits for degree values
*               from the CMD task, retrieves the corresponding cosine value from the lookup table, and
*               sends the result back to the CMD task.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskCOS()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
static void App_TaskCOS(void *p_arg)
{

  /* prevent compiler warnings */
  (void)p_arg;

  OS_MSG_SIZE cmd_msg_size = 0;
  OS_ERR os_err;
  CPU_TS ts;
  CPU_INT32U degree_val = 0;

  while (DEF_TRUE)
  {
    // wait for degree value from CMD task. Pending indefinitely.
    CPU_INT32U *cos_degree = (CPU_INT32U *)OSTaskQPend(0, OS_OPT_PEND_BLOCKING, &cmd_msg_size, &ts, &os_err);

    if (NULL != cos_degree)
    {
      degree_val = *cos_degree; // get degree value if valid value extracted from task queue
    }
    else
    {
      degree_val = 0; // post cos(0) if error occurs
    }

    // post cosine value back to CMD task.
    OSQPost(&g_msg_queue_cos_values, &g_out_cos_values[degree_val],
            sizeof(g_out_cos_values[degree_val]), OS_OPT_POST_FIFO, &os_err);

    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
  }
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

/*
*********************************************************************************************************
*                                          App_TaskCOM()
*
* Description : COM Task checks for available bytes within the UART receive buffer. If correct string is
*               available (e.g. PC -> uC: #abc$ or #Hellor World$), process the message and output a
*               pre-defined string via UART and append the user-defined string via the UART interface.
*               The COM task sends received commands to the CMD task and waits for output messages from
*               the CMD task to send them via UART.
*
* Argument(s) : p_arg   is the argument passed to 'App_TaskCOM()' by 'OSTaskCreate()'.
*
* Return(s)   : none
*
* Note(s)     : none
*********************************************************************************************************
*/
static void App_TaskCOM(void *p_arg)
{
  /* declare and define task local variables */
  OS_ERR os_err;
  CPU_INT08U rec_byte = 0x00;
  CPU_INT08U rx_msg[UART_1_RX_BUFFER_SIZE] = {0};
  CPU_INT08U idx = 0x00;
  CPU_INT08U rec_byte_cnt = 0x00;
  CPU_BOOLEAN str_available = DEF_FALSE;
  CPU_INT08U msg_idx = 0x00;

  CPU_CHAR *output_msg = NULL;
  CPU_TS ts;
  OS_MSG_SIZE msg_size;

  /* prevent compiler warnings */
  (void)p_arg;
  (void)Start_of_Packet;
  (void)End_of_Packet;

  /* start of the endless loop */
  while (DEF_TRUE)
  {
    /* check if a byte is available */
    rec_byte = uart_get_byte();
    /* check if the received byte is '#'*/
    if (rec_byte == Start_of_Packet && DEF_FALSE == str_available)
    {
      /* if received byte was correct */
      while (DEF_TRUE)
      {
        /* receive byte by byte */
        rec_byte = uart_get_byte();
        /* check is byte is something meaningful */
        if (rec_byte)
        {
          /* save byte into software receive buffer and increment idx */
          rx_msg[idx++] = rec_byte;
        }
        /* initiate scheduler */
        OSTimeDlyHMSM(0, 0, 0, 20, OS_OPT_TIME_HMSM_STRICT, &os_err);

        /* check if received byte is '$' */
        if (rx_msg[idx - 1] == End_of_Packet)
        {
          /* if end of packet is reached -> break */
          break;
        }
      }

      rx_msg[idx - 1] = '\0'; // terminate the input string

      /* message received, calculate received bytes, -2 because of '#' & '$' */
      rec_byte_cnt = idx - 2;

      // do not accept more bytes than expected
      if (rec_byte_cnt > MAX_INPUT_STRING_LENGTH)
      {
        ResetCOMState(&rx_msg[0], sizeof(rx_msg), &str_available, &rec_byte, &idx);
      }
      else
      {
        /* signal that a string is available */
        str_available = DEF_TRUE;
      }
    }
    /* if received byte wasn't start of packet */
    else if (DEF_FALSE == str_available)
    {
      ResetCOMState(&rx_msg[0], sizeof(rx_msg), &str_available, &rec_byte, &idx);
    }
    /* check if message is available */
    if (str_available)
    {
      CPU_INT08U idx_msg = msg_idx % MAX_AMOUNT_INPUT_STRINGS;

      strncpy(g_in_msgs[idx_msg], (CPU_CHAR *)rx_msg, 8);

      // post message to CMD task. Validity of message is checked by CMD task.
      OSTaskQPost(&App_TaskCMD_TCB, &g_in_msgs[idx_msg], strlen(g_in_msgs[idx_msg]) + 1, OS_OPT_POST_FIFO, &os_err);

      // Queue is not full. Message written. Get another one.
      if (OS_ERR_Q_MAX != os_err)
      {
        ResetCOMState(&rx_msg[0], sizeof(rx_msg), &str_available, &rec_byte, &idx);
        msg_idx++;
      }
    }

    // Check if the CMD task notified a message to be sent via UART. Non-blocking pend.
    output_msg = (CPU_CHAR *)OSTaskQPend(COM_TASK_CMD_RX_TIMEOUT_MS, OS_OPT_PEND_NON_BLOCKING, &msg_size, &ts, &os_err);

    if (NULL != output_msg)
    {
      // CMD task has sent an output message to be sent via UART.

      const CPU_INT32U real_msg_length = strlen(output_msg);

      // send output message via UART byte by byte
      for (CPU_INT08U jdx = 0x00; jdx < real_msg_length; jdx++)
      {
        uart_send_byte(output_msg[jdx]);
      }
    }

    /* initiate scheduler */
    OSTimeDlyHMSM(0, 0, 0, GENERAL_TASK_DELAY_MS, OS_OPT_TIME_HMSM_STRICT, &os_err);
  }
}

/* END OF FILE */