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

#ifndef __AP_TIMER_H__
#define __AP_TIMER_H__

#include "OSAL.h"
#include "types.h"

//ap timer interrupt process function.
void __attribute__((weak)) AP_TIMER_IRQHandler(void);

//enable ap timer interrupt.
void ap_timer_init(void);

//disable ap timer interrupt and disable its clock.
void ap_timer_deinit(void);

//set timer registers,ready to run.
//timeId should form 1 to 4.time mutiple four should not more than 0xffffff.
void set_ap_timer(uint8  timeId, int time);

//clear timer registers,stop it.
void clear_ap_timer(uint8 timeId);


void mask_ap_timer_int(uint8  timeId, uint8_t en);
void set_ap_timer_enable(uint8  timeId, uint8_t en);

#endif

