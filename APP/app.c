/*
*********************************************************************************************************
*
*                                       MES1 Embedded Software (RTOS)
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : Joaquin Ortiz, Ervin Sejdovic, Ana Zabolotneva, Michael Frank

*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <includes.h>
#include <cyapicallbacks.h>

#include <thumbstick_task.h>
#include <dshot_task.h>
#include <com_task.h>


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

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/
static OS_TCB App_TaskStartTCB;
static CPU_STK App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
static void App_TaskStart(void *p_arg);
static void App_TaskCreate(void);
static void App_ObjCreate(void);


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

  /* create Thumstick Task channel*/
  OSTaskCreate((OS_TCB *)GetThumbstickTaskTCB(),
               (CPU_CHAR *)"TaskThumbstick",
               (OS_TASK_PTR)App_TaskThumbstick,
               (void *)0,
               (OS_PRIO)APP_CFG_TASK_THUMSTICK_PRIO,
               (CPU_STK *)GetThumbstickTaskStk(),
               (CPU_STK_SIZE)APP_CFG_TASK_THUMBSTICK_STK_SIZE_LIMIT,
               (CPU_STK_SIZE)APP_CFG_TASK_THUMBSTICK_STK_SIZE,
               (OS_MSG_QTY)TASK_QUEUE_LENGTH,
               (OS_TICK)0u,
               (void *)0,
               (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR *)&os_err);
  /* create Dshot Task channel*/
  OSTaskCreate((OS_TCB *)GetDshotTaskTCB(),
               (CPU_CHAR *)"TaskDshot",
               (OS_TASK_PTR)App_TaskDshot,
               (void *)0,
               (OS_PRIO)APP_CFG_TASK_DSHOT_PRIO,
               (CPU_STK *)GetDshotTaskStk(),
               (CPU_STK_SIZE)APP_CFG_TASK_DSHOT_STK_SIZE_LIMIT,
               (CPU_STK_SIZE)APP_CFG_TASK_DSHOT_STK_SIZE,
               (OS_MSG_QTY)DSHOT_TASK_QUEUE_LENGTH,
               (OS_TICK)0u,
               (void *)0,
               (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR *)&os_err);
  /* create COM task */
  OSTaskCreate((OS_TCB *)GetComTaskTCB(),
               (CPU_CHAR *)"TaskCom",
               (OS_TASK_PTR)App_TaskCom,
               (void *)0,
               (OS_PRIO)APP_CFG_TASK_COM_PRIO,
               (CPU_STK *)GetComTaskStk(),
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

  OSSemCreate(GetNewThrottleEventSem(),"SEM_NEW_THROTTLE_EVENT",0,&os_err);
}

/* END OF FILE */