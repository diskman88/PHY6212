/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     gcc_csky.ld
 * @brief    csky linker file
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
MEMORY
{
    LR_IROM0   : ORIGIN = 0x1105D000, LENGTH = 0x20000    /* FLASH 120 KB*/
    LR_IRAM0   : ORIGIN = 0x1fff0000, LENGTH = 0x00800    /* on-chip SRAM0 2KB*/
    LR_IRAM1   : ORIGIN = 0x1fff0800, LENGTH = 0x00400    /* on-chip SRAM0 1KB*/
    LR_IRAM2   : ORIGIN = 0x1fff0c00, LENGTH = 0x00400    /* on-chip SRAM0 1KB*/
    LR_IRAM3   : ORIGIN = 0x1fff1000, LENGTH = 0x03800    /* on-chip SRAM0 14KB*/
    LR_IRAM4   : ORIGIN = 0x1fff4800, LENGTH = 0x1E000    /* on-chip SRAM 120KB*/
}

__min_heap_size = 0x800;
PROVIDE (__ram_end  = 0x1fff4800 + 0x1E000);
PROVIDE (__heap_end = __ram_end);

REGION_ALIAS("REGION_TEXT",    LR_IROM0);
REGION_ALIAS("REGION_RODATA",  LR_IROM0);
REGION_ALIAS("REGION_RETENTION0",    LR_IRAM4);
REGION_ALIAS("REGION_RETENTION1",    LR_IRAM4);
REGION_ALIAS("REGION_NORETENTION0",  LR_IRAM4);
REGION_ALIAS("REGION_NORETENTION1",  LR_IRAM4);
REGION_ALIAS("REGION_NORETENTION2",  LR_IRAM4);

ENTRY(Reset_Handler)
SECTIONS
{
 .data_text : {
  *startup.o(*.startup)
  KEEP(*startup.o(.vectors))
  *pm.o(.text* .scommon COMMON)
  *libcsi.a:lpm_arch_reg_save.o(.text)
  *k_obj.o(.data* .*bss* .scommon COMMON)
  *libcsi.a:power_manager.o(.text.*)
  *libcsi.a:spif.o(.text.*)
  *libcsi.a:dw_iic.o(.text.*)
  *libcsi.a:isr.o(.text.*)
  *libcsi.a:phy_pmu.o(.text.*)
  *libcsi.a:systick.o(.text.*)
  *libcsi.a:hook_impl.o(.text.*)
  *libcsi.a:hook_weak.o(.text.*)
  *libcsi.a:k_buf_queue.o(.text.*)
  *libcsi.a:k_idle.o(.text.*)
  *libcsi.a:k_ringbuf.o(.text.*)
  *libcsi.a:k_mutex.o(.text.*)
  *libcsi.a:k_pend.o(.text.*)
  *libcsi.a:k_sched.o(.text.*)
  *libcsi.a:k_sem.o(.text.*)
  *libcsi.a:k_tick.o(.text.*)
  *libcsi.a:k_timer.o(.text.*)
  *libcsi.a:k_time.o(.text.*)
  *libcsi.a:common.o(.text.*)
  *libcsi.a:malloc.o(.text.*)
  *libcsi.a:k_sys.o(.text.*)
  *libcsi.a:spif.o(.text.*)
  *libcsi.a:port_s.o(.text*)
  *libcsi.a:pinmux.o(.text*)
  *libgcc.a:_thumb1_case_*.o(.text*)
  *liblibs.a:(.text.memcpy .text.memset .text.memcmp)
  *libbt_host.a:hci_core.o(.text.*)
  *libbt_host.a:h4_driver.o(.text.*)
  *libbt_host.a:buf.o(.text.*)
  *libbt_host.a:ble.o(.text.*)
  *libdrivers_bt*.a:(.text.* .rodata.*)

 } > REGION_RETENTION1
 .text : {
  . = ALIGN(0x4) ;
  __stext = . ;
  *(.text)
  *(.text*)
  *(.text.*)
  *(.gnu.warning)
  *(.stub)
  *(.gnu.linkonce.t*)
  *(.glue_7t)
  *(.glue_7)
  *(.jcr)
  KEEP (*(.init))
  KEEP (*(.fini))
  . = ALIGN (4) ;
  PROVIDE(__ctbp = .);
  *(.call_table_data)
  *(.call_table_text)
  . = ALIGN(0x10) ;
  __etext = . ;
 } > REGION_TEXT
 .eh_frame_hdr : {
  *(.eh_frame_hdr)
 } > REGION_TEXT 
 .eh_frame : ONLY_IF_RO {
  KEEP (*(.eh_frame))
 } > REGION_TEXT
 .gcc_except_table : ONLY_IF_RO {
  *(.gcc_except_table .gcc_except_table.*)
 } > REGION_TEXT
 .rodata : {
  . = ALIGN(0x4) ;
  __srodata = .;
  *(.rdata)
  *(.rdata*)
  *(.rdata1)
  *(.rdata.*)
  *(.rodata)
  *(.rodata1)
  *(.rodata*)
  *(.rodata.*)
  *(.rodata.str1.4)
  . = ALIGN(0x4) ;
  __ctor_start__ = .;
  KEEP (*(SORT(.ctors.*)))
  KEEP (*(.ctors))
  __ctor_end__ = .;
  KEEP (*(SORT(.dtors.*)))
  KEEP (*(.dtors))
  __dtor_end__ = .;
   _bt_settings_start = .;
   KEEP(*(._bt_settings.static.*))
   _bt_settings_end = .;
   . = ALIGN(0x4) ;
  __erodata = .;
  __rodata_end__ = .;
 } > REGION_RODATA
 .data : {
  . = ALIGN(0x4) ;
  __sdata = . ;
  __data_start__ = . ;
  __data_copy_start__ = .;
  data_start = . ;
  *(.got.plt)
  *(.got)
  *(.gnu.linkonce.r*)
  *(.data)
  *(.data*)
  *(.data1)
  *(.data.*)
  *(.gnu.linkonce.d*)
  *(.data1)
  *(.gcc_except_table)
  *(.gcc_except_table*)
  __start_init_call = .;
  *(.initcall.init)
  __stop_init_call = .;
  __start_cmd = .;
  *(.bootloaddata.cmd)
  . = ALIGN(4) ;
  __stop_cmd = .;
  *(.sdata)
  *(.sdata.*)
  *(.gnu.linkonce.s.*)
  *(__libc_atexit)
  *(__libc_subinit)
  *(__libc_subfreeres)
  *(.note.ABI-tag)
  . = ALIGN(0x4) ;
  _net_buf_pool_list = .;
  KEEP(*(SORT_BY_NAME("._net_buf_pool.static.*")))
  _net_buf_pool_list_end = .;
  . = ALIGN(0x4) ;
  __edata = .;
  __data_end__ = .;
  . = ALIGN(0x4) ;
 } > REGION_RETENTION1
 .eh_frame : ONLY_IF_RW {
  KEEP (*(.eh_frame))
 } > REGION_RETENTION1
 .gcc_except_table : ONLY_IF_RW {
  *(.gcc_except_table .gcc_except_table.*)
  __edata = .;
  __data_end__ = .;
  __data_copy_end__ = .;
 } > REGION_RETENTION1
 .jmp_table : {
  KEEP (*(jump_table_mem_area))
 } > LR_IRAM1
 .global_config : {
  KEEP (*(global_config_area))
 } > LR_IRAM2
 .bss : {
  . = ALIGN(0x4) ;
  __sbss = ALIGN(0x4) ;
  __bss_start__ = . ;
  *(.dynsbss)
  *(.sbss)
  *(.sbss.*)
  *(.scommon)
  *(.dynbss)
  *(.bss)
  *(.bss.*)
  *(COMMON)
  . = ALIGN(0x4) ;
  __ebss = . ;
  __bss_end__ = .;
  __end = . ;
  end = . ;
 } > REGION_RETENTION0
  .data_noretention : {
  . = ALIGN(0x4) ;
  *(noretention_mem_area0)
 } > REGION_NORETENTION0
 ._user_heap : {
  . = ALIGN(0x4) ;
  __heap_start = .;
  . += __min_heap_size;
  . = ALIGN(0x4) ;
 } > REGION_NORETENTION0
}
