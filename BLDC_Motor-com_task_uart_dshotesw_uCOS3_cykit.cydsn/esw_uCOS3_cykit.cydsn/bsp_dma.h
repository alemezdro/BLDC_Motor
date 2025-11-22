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

#include <CyDmac.h>

CPU_INT08U init_dma(CPU_INT08U burst_count, CPU_INT08U request_per_burst, 
                  CPU_INT16U upper_src_addr, CPU_INT16U upper_dest_addr);

CPU_INT08U dma_td_allocate(CPU_VOID);

cystatus dma_td_set_configuration(CPU_INT08U td_handle, CPU_INT16U transfer_cnt, 
                                  CPU_INT08U next_td, CPU_INT08U configuration);

cystatus dma_td_set_address(CPU_INT08U td_handle, CPU_INT16U source, 
                              CPU_INT16U destination);

cystatus dma_ch_set_init_td(CPU_INT08U ch_handle, CPU_INT08U start_td);

cystatus dma_ch_enable(CPU_INT08U ch_handle, CPU_INT08U preserve_tds);

cystatus dma_ch_set_request(CPU_INT08U ch_handle, CPU_INT08U request);

cystatus dma_ch_status(CPU_INT08U ch_handle, CPU_INT08U* current_td, CPU_INT08U* state);

cystatus dma_ch_disable(CPU_INT08U ch_handle);



/* [] END OF FILE */
