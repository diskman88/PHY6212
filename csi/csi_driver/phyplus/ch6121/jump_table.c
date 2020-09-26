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


/**************************************************************************************************
  Filename:       jump_table.c
  Revised:
  Revision:

  Description:    Jump table that holds function pointers and veriables used in ROM code.


**************************************************************************************************/

/*******************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include "jump_function.h"
#include "global_config.h"
#include "OSAL_Tasks.h"
#include "rf_phy_driver.h"
#include "pwrmgr.h"
#include "adc.h"
#include "gpio.h"
#include "uart.h"
#include "i2c_common.h"
//#include "kscan.h"
#include "rflib.h"
#include "log.h"
#include "spi.h"
#include "watchdog.h"
#include "ap_timer.h"
#include "lib_printf.h"

extern void PendSV_Handler(void);
extern void CSI_SysTick_Handler(void);

extern void llConnTerminate1(llConnState_t *connPtr, uint8 reason);
extern void llPduLengthUpdate1(uint16 connHandle);
extern void llMasterEvt_TaskEndOk1(void);
extern uint8 llSetupNextMasterEvent1(void);
extern uint8 llProcessMasterControlProcedures1(llConnState_t *connPtr);
extern void llProcessMasterControlPacket1(llConnState_t *connPtr, uint8  *pBuf);

extern void llSlaveEvt_TaskEndOk1(void);
extern uint8 llSetupNextSlaveEvent1(void);
extern uint8 llProcessSlaveControlProcedures1(llConnState_t *connPtr);
extern void llProcessTxData1(llConnState_t *connPtr, uint8 context);
extern uint8 llProcessRxData1(void);
extern void LL_IRQHandler1(void);
extern uint8 LL_SetScanControl1( uint8 scanMode,uint8 filterReports );
extern uint8 LL_SetAdvControl1(uint8 advMode);

extern uint8 llSetupAdv1( void );
extern uint8 LL_CreateConn1( uint16 scanInterval,
                          uint16 scanWindow,
                          uint8  initWlPolicy,
                          uint8  peerAddrType,
                          uint8  *peerAddr,
                          uint8  ownAddrType,
                          uint16 connIntervalMin,
                          uint16 connIntervalMax,
                          uint16 connLatency,
                          uint16 connTimeout,
                          uint16 minLength,
                          uint16 maxLength );

extern void CSI_UART0_IRQHandler(void);
extern void CSI_GPIO_IRQHandler(void);
extern int CSI_TIM0_IRQHandler(void);
extern void CSI_SPI0_IRQHandler(void);
extern void CSI_SPI1_IRQHandler(void);
extern void CSI_AP_TIMER_IRQHandler(void);
extern void CSI_RTC_IRQHandler(void);
extern void CSI_WDT_IRQHandler(void);
extern void CSI_IIC0_IRQHandler(void);
extern void CSI_IIC1_IRQHandler(void);
extern int32_t arch_resume_context(void);
extern int TIM0_IRQHandler1(void);
extern void LL_master_conn_event1(void);
extern void __attribute__((used)) phy_ADC_VoiceIRQHandler(void);
extern void ll_hw_go1(void);

/*******************************************************************************
 * MACROS
 */
void (*trap_c_callback)(void);

extern const uint32_t *const jump_table_base[];
void _hard_fault(uint32_t *arg)
{
    uint32_t *stk = (uint32_t *)((uint32_t)arg);

    printk("[Hard fault handler]\n");
    printk("R0  = 0x%x\n", stk[9]);
    printk("R1  = 0x%x\n", stk[10]);
    printk("R2  = 0x%x\n", stk[11]);
    printk("R3  = 0x%x\n", stk[12]);
    printk("R4  = 0x%x\n", stk[1]);
    printk("R5  = 0x%x\n", stk[2]);
    printk("R6  = 0x%x\n", stk[3]);
    printk("R7  = 0x%x\n", stk[4]);
    printk("R8  = 0x%x\n", stk[5]);
    printk("R9  = 0x%x\n", stk[6]);
    printk("R10 = 0x%x\n", stk[7]);
    printk("R11 = 0x%x\n", stk[8]);
    printk("R12 = 0x%x\n", stk[13]);
    printk("SP  = 0x%x\n", stk[0]);
    printk("LR  = 0x%x\n", stk[14]);
    printk("PC  = 0x%x\n", stk[15]);
    printk("PSR = 0x%x\n", stk[16]);
    printk("HFSR = 0x%x\n", *(volatile uint32_t *)0xE000ED2C);

    if (trap_c_callback) {
        trap_c_callback();
    }

    while (1);
}

#ifdef __GNUC__
__attribute__((naked)) void hard_fault(void)
{
    asm("ldr     r1, =0xFFFFFFFD\n\r"
        "ldr     r0, [sp, #4]\n\r"
        "cmp     r1, r0\n\r"
        "beq     .Lstore_in_psp\n\r"
        "mov     r0, sp\n\r"
        "add     r0, r0, #8\n\r"
        "sub     r0, r0, #0x24\n\r"
        "mov     sp, r0\n\r"
        "b       .Lstore_in_msp\n\r"
        ".Lstore_in_psp:\n\r"
        "mrs     r0, psp\n\r"
        "sub     r0, r0, #0x24\n\r"
        ".Lstore_in_msp:\n\r"
        "mov     r1, r0\n\r"
        "add     r1, r1, #0x44\n\r"
        "str     r1, [r0]\n\r"
        "add     r0, r0, #4\n\r"
        "stmia   r0!, {r4-r7}\n\r"
        "mov     r4, r8\n\r"
        "mov     r5, r9\n\r"
        "mov     r6, r10\n\r"
        "mov     r7, r11\n\r"
        "stmia   r0!, {r4-r7}\n\r"
        "sub     r0, r0, #36\n\r"
        "ldr     r1, =_hard_fault\n\r"
        "bx      r1\n\r");
}
#else
__asm void hard_fault(void)
{
    IMPORT  _hard_fault
    ldr     r1, = 0xFFFFFFFD
                  ldr     r0, [sp, #4]
                  cmp     r1, r0
                  beq     Lstore_in_psp
                  mov     r0, sp
                  adds    r0, r0, #8
                  subs    r0, r0, #0x24
                  mov     sp, r0
                  b       Lstore_in_msp
                  Lstore_in_psp
                  mrs     r0, psp
                  subs    r0, r0, #0x24
                  Lstore_in_msp
                  mov     r1, r0
                  adds    r1, r1, #0x44
                  str     r1, [r0]
                  adds    r0, r0, #4
                  stmia   r0!, {r4 - r7}
                  mov     r4, r8
                  mov     r5, r9
                  mov     r6, r10
                  mov     r7, r11
                  stmia   r0!, {r4 - r7}
                  subs    r0, r0, #32
                  subs    r0, r0, #36
                  ldr     r1, = _hard_fault
                                bx      r1
}
#endif

/*******************************************************************************
 * CONSTANTS
 */
// jump table, this table save the function entry which will be called by ROM code
// item 1 - 4 for OSAL task entry
// item 224 - 255 for ISR(Interrupt Service Routine) entry
// others are reserved by ROM code
const uint32_t *const jump_table_base[256] __attribute__((section("jump_table_mem_area"))) = {0};

/*******************************************************************************
 * Prototypes
 */


/*******************************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * EXTERNAL VARIABLES
 */
uint32 global_config[SOFT_PARAMETER_NUM] __attribute__((section("global_config_area")));


