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

#include <bsp_spi.h>

/* BSP Functions for SPI */


CPU_VOID init_spi(CPU_VOID) 
{
  //SPIM_1_Init();
  SPIM_1_Start();
}

CPU_VOID spi_set_tx_int_mode(CPU_INT08U int_src)
{
  SPIM_1_SetTxInterruptMode(int_src);
}

CPU_VOID spi_set_rx_int_mode(CPU_INT08U int_src)
{
  SPIM_1_SetRxInterruptMode(int_src);
}

CPU_INT08U spi_get_byte(CPU_VOID)
{
  return SPIM_1_ReadRxData();
}

CPU_VOID spi_send_byte(CPU_INT08U byte)
{
  SPIM_1_WriteTxData(byte);
}

CPU_INT08U spi_get_rx_buffer_size(CPU_VOID)
{
  return SPIM_1_GetRxBufferSize(); 
}

CPU_INT08U spi_get_tx_buffer_size(CPU_VOID)
{
  return SPIM_1_GetTxBufferSize();
}

CPU_INT08U spi_get_rx_status(CPU_VOID)
{
  return SPIM_1_ReadRxStatus();
}

CPU_INT08U spi_get_tx_status(CPU_VOID)
{
  return SPIM_1_ReadTxStatus();
}

/* [] END OF FILE */
