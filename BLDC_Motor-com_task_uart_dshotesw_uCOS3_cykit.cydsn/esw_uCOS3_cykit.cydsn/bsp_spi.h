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

#include <cpu.h>

#include <SPIM_1.h>

CPU_VOID init_spi(CPU_VOID);

CPU_VOID spi_set_tx_int_mode(CPU_INT08U int_src);
CPU_VOID spi_set_rx_int_mode(CPU_INT08U int_src);

CPU_INT08U spi_get_rx_buffer_size(CPU_VOID);
CPU_INT08U spi_get_tx_buffer_size(CPU_VOID);

CPU_INT08U spi_get_rx_status(CPU_VOID);
CPU_INT08U spi_get_tx_status(CPU_VOID);

CPU_INT08U spi_get_byte(CPU_VOID);
CPU_VOID spi_send_byte(CPU_INT08U byte);

/* [] END OF FILE */
