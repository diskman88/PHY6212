/**************************************************************************************************

  Phyplus Microelectronics Limited confidential and proprietary.
  All rights reserved.

  IMPORTANT: All rights of this software belong to Phyplus Microelectronics
  Limited ("Phyplus"). Your use of this Software is limited to those
  specific rights granted under  the terms of the business contract, the
  confidential agreement, the non-disclosure agreement and any other forms
  of agreements as a customer or a partner of Phyplus. You may not use this
  Software unless you agree to abide by the terms of these agreements.
  You acknowledge that the Software may not be modified, copied,
  distributed or disclosed unless embedded on a Phyplus Bluetooth Low Energy
  (BLE) integrated circuit, either as a product or is integrated into your
  products.  Other than for the aforementioned purposes, you may not use,
  reproduce, copy, prepare derivative works of, modify, distribute, perform,
  display or sell this Software and/or its documentation for any purposes.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  PHYPLUS OR ITS SUBSIDIARIES BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

**************************************************************************************************/

/*******************************************************************************
* @file		common.c
* @brief	Contains all functions support for common function driver£¬such as string function,you can use this driver for spi,adc,uart and so on
* @version	0.0
* @date		18. Oct. 2017
* @author	qing.han
*
* Copyright(C) 2016, PhyPlus Semiconductor
* All rights reserved.
*
*******************************************************************************/

#include "types.h"
#include "common.h"
#include "ap_cp.h"

/**************************************************************************************
 * @fn          subWriteReg
 *
 * @brief       This function process for write register with sub bit
 *
 * input parameters
 *
 * @param       uint32_t addr: register address
 *              uint8_t hOff: high bit offset
 *              uint8_t lOff: low bit offset
 *              uint32_t value: write value
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 **************************************************************************************/
void subWriteReg(uint32_t addr, uint8_t hOff, uint8_t lOff, uint32_t value)
{
    uint32_t temp = read_reg(addr);
    uint32_t temp2 = 0xffffffff;
    uint32_t temp3 = value << lOff;

    for (uint8_t i = lOff; i <= hOff; i++) {
        temp2 &= ~BIT(i);
    }

    temp = temp & temp2;
    temp = temp | temp3;
    write_reg(addr, temp);
}

