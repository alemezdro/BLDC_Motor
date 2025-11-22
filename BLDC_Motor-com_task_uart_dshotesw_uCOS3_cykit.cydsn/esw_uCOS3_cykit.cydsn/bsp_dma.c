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

#include <bsp_dma.h>

#include <DMA_1_dma.h>

CPU_INT08U init_dma(CPU_INT08U burst_count, CPU_INT08U request_per_burst, 
                  CPU_INT16U upper_src_addr, CPU_INT16U upper_dest_addr){
  return DMA_1_DmaInitialize(burst_count, request_per_burst, upper_src_addr, upper_dest_addr);
}

CPU_INT08U dma_td_allocate(CPU_VOID) {
  return CyDmaTdAllocate();
}

cystatus dma_td_set_configuration(CPU_INT08U td_handle, CPU_INT16U transfer_cnt, 
                                  CPU_INT08U next_td, CPU_INT08U configuration){
  return CyDmaTdSetConfiguration(td_handle, transfer_cnt, next_td, configuration);
}

cystatus dma_td_set_address(CPU_INT08U td_handle, CPU_INT16U source, 
                              CPU_INT16U destination){
  return CyDmaTdSetAddress(td_handle, source, destination);                              
}

cystatus dma_ch_set_init_td(CPU_INT08U ch_handle, CPU_INT08U start_td){
  return CyDmaChSetInitialTd(ch_handle, start_td);
}

cystatus dma_ch_enable(CPU_INT08U ch_handle, CPU_INT08U preserve_tds){
  return CyDmaChEnable(ch_handle, preserve_tds);
}

cystatus dma_ch_set_request(CPU_INT08U ch_handle, CPU_INT08U request){
  return CyDmaChSetRequest(ch_handle, request);
}

cystatus dma_ch_status(CPU_INT08U ch_handle, CPU_INT08U* current_td, 
                      CPU_INT08U* state){
return CyDmaChStatus(ch_handle, current_td, state);                      
}

cystatus dma_ch_disable(CPU_INT08U ch_handle){
  return CyDmaChDisable(ch_handle);
}

/* [] END OF FILE */
