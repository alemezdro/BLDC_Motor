/*******************************************************************************
* File Name: pin_rgb_r.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_pin_rgb_r_H) /* Pins pin_rgb_r_H */
#define CY_PINS_pin_rgb_r_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "pin_rgb_r_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 pin_rgb_r__PORT == 15 && ((pin_rgb_r__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    pin_rgb_r_Write(uint8 value);
void    pin_rgb_r_SetDriveMode(uint8 mode);
uint8   pin_rgb_r_ReadDataReg(void);
uint8   pin_rgb_r_Read(void);
void    pin_rgb_r_SetInterruptMode(uint16 position, uint16 mode);
uint8   pin_rgb_r_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the pin_rgb_r_SetDriveMode() function.
     *  @{
     */
        #define pin_rgb_r_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define pin_rgb_r_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define pin_rgb_r_DM_RES_UP          PIN_DM_RES_UP
        #define pin_rgb_r_DM_RES_DWN         PIN_DM_RES_DWN
        #define pin_rgb_r_DM_OD_LO           PIN_DM_OD_LO
        #define pin_rgb_r_DM_OD_HI           PIN_DM_OD_HI
        #define pin_rgb_r_DM_STRONG          PIN_DM_STRONG
        #define pin_rgb_r_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define pin_rgb_r_MASK               pin_rgb_r__MASK
#define pin_rgb_r_SHIFT              pin_rgb_r__SHIFT
#define pin_rgb_r_WIDTH              1u

/* Interrupt constants */
#if defined(pin_rgb_r__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in pin_rgb_r_SetInterruptMode() function.
     *  @{
     */
        #define pin_rgb_r_INTR_NONE      (uint16)(0x0000u)
        #define pin_rgb_r_INTR_RISING    (uint16)(0x0001u)
        #define pin_rgb_r_INTR_FALLING   (uint16)(0x0002u)
        #define pin_rgb_r_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define pin_rgb_r_INTR_MASK      (0x01u) 
#endif /* (pin_rgb_r__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define pin_rgb_r_PS                     (* (reg8 *) pin_rgb_r__PS)
/* Data Register */
#define pin_rgb_r_DR                     (* (reg8 *) pin_rgb_r__DR)
/* Port Number */
#define pin_rgb_r_PRT_NUM                (* (reg8 *) pin_rgb_r__PRT) 
/* Connect to Analog Globals */                                                  
#define pin_rgb_r_AG                     (* (reg8 *) pin_rgb_r__AG)                       
/* Analog MUX bux enable */
#define pin_rgb_r_AMUX                   (* (reg8 *) pin_rgb_r__AMUX) 
/* Bidirectional Enable */                                                        
#define pin_rgb_r_BIE                    (* (reg8 *) pin_rgb_r__BIE)
/* Bit-mask for Aliased Register Access */
#define pin_rgb_r_BIT_MASK               (* (reg8 *) pin_rgb_r__BIT_MASK)
/* Bypass Enable */
#define pin_rgb_r_BYP                    (* (reg8 *) pin_rgb_r__BYP)
/* Port wide control signals */                                                   
#define pin_rgb_r_CTL                    (* (reg8 *) pin_rgb_r__CTL)
/* Drive Modes */
#define pin_rgb_r_DM0                    (* (reg8 *) pin_rgb_r__DM0) 
#define pin_rgb_r_DM1                    (* (reg8 *) pin_rgb_r__DM1)
#define pin_rgb_r_DM2                    (* (reg8 *) pin_rgb_r__DM2) 
/* Input Buffer Disable Override */
#define pin_rgb_r_INP_DIS                (* (reg8 *) pin_rgb_r__INP_DIS)
/* LCD Common or Segment Drive */
#define pin_rgb_r_LCD_COM_SEG            (* (reg8 *) pin_rgb_r__LCD_COM_SEG)
/* Enable Segment LCD */
#define pin_rgb_r_LCD_EN                 (* (reg8 *) pin_rgb_r__LCD_EN)
/* Slew Rate Control */
#define pin_rgb_r_SLW                    (* (reg8 *) pin_rgb_r__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define pin_rgb_r_PRTDSI__CAPS_SEL       (* (reg8 *) pin_rgb_r__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define pin_rgb_r_PRTDSI__DBL_SYNC_IN    (* (reg8 *) pin_rgb_r__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define pin_rgb_r_PRTDSI__OE_SEL0        (* (reg8 *) pin_rgb_r__PRTDSI__OE_SEL0) 
#define pin_rgb_r_PRTDSI__OE_SEL1        (* (reg8 *) pin_rgb_r__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define pin_rgb_r_PRTDSI__OUT_SEL0       (* (reg8 *) pin_rgb_r__PRTDSI__OUT_SEL0) 
#define pin_rgb_r_PRTDSI__OUT_SEL1       (* (reg8 *) pin_rgb_r__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define pin_rgb_r_PRTDSI__SYNC_OUT       (* (reg8 *) pin_rgb_r__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(pin_rgb_r__SIO_CFG)
    #define pin_rgb_r_SIO_HYST_EN        (* (reg8 *) pin_rgb_r__SIO_HYST_EN)
    #define pin_rgb_r_SIO_REG_HIFREQ     (* (reg8 *) pin_rgb_r__SIO_REG_HIFREQ)
    #define pin_rgb_r_SIO_CFG            (* (reg8 *) pin_rgb_r__SIO_CFG)
    #define pin_rgb_r_SIO_DIFF           (* (reg8 *) pin_rgb_r__SIO_DIFF)
#endif /* (pin_rgb_r__SIO_CFG) */

/* Interrupt Registers */
#if defined(pin_rgb_r__INTSTAT)
    #define pin_rgb_r_INTSTAT            (* (reg8 *) pin_rgb_r__INTSTAT)
    #define pin_rgb_r_SNAP               (* (reg8 *) pin_rgb_r__SNAP)
    
	#define pin_rgb_r_0_INTTYPE_REG 		(* (reg8 *) pin_rgb_r__0__INTTYPE)
#endif /* (pin_rgb_r__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_pin_rgb_r_H */


/* [] END OF FILE */
