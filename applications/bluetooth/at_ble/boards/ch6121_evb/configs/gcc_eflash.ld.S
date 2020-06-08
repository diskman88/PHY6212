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
    LR_IROM0   : ORIGIN = 0x1105D000, LENGTH = 0x20000    /* FLASH 128KB*/
    LR_IRAM0   : ORIGIN = 0x1fff0000, LENGTH = 0x00800    /* on-chip SRAM0 2KB*/
    LR_IRAM1   : ORIGIN = 0x1fff0800, LENGTH = 0x00400    /* on-chip SRAM0 1KB*/
    LR_IRAM2   : ORIGIN = 0x1fff0c00, LENGTH = 0x00400    /* on-chip SRAM0 1KB*/
    LR_IRAM3   : ORIGIN = 0x1fff1000, LENGTH = 0x03000    /* on-chip SRAM0 12KB*/
    LR_IRAM4   : ORIGIN = 0x1fff4800, LENGTH = 0x0B800    /* on-chip SRAM0 + SRAM1 46KB*/
    LR_IRAM5   : ORIGIN = 0x20000000, LENGTH = 0x12800    /* on-chip SRAM2 + SRAM3 + SRAM4 74KB*/
}

__min_heap_size = 0x200;
PROVIDE (__ram_end  = 0x1fff4800 + 0x0B800);
PROVIDE (__heap_end = __ram_end);

REGION_ALIAS("REGION_TEXT",    LR_IROM0);
REGION_ALIAS("REGION_RODATA",  LR_IROM0);
REGION_ALIAS("REGION_RETENTION0",    LR_IRAM4);
REGION_ALIAS("REGION_RETENTION1",    LR_IRAM5);
REGION_ALIAS("REGION_NORETENTION0",  LR_IRAM4);

ENTRY(Reset_Handler)
SECTIONS
{
 .data_text : {
  /* DO NOT MODIFY SECTIONS BELOW!!!!!!!!! */
  *startup.o(*.startup)
  KEEP(*startup.o(.vectors))
 /* rhino kernel */
  *libcsi.a:k_obj.o(.text.* .data* .*bss* .scommon COMMON)
  *libcsi.a:k_pend.o(.text.*)
  *libcsi.a:k_ringbuf.o(.text.*)
  *libcsi.a:k_sched.o(.text.*)
  *libcsi.a:k_idle.o(.text.*)
  *libcsi.a:k_buf_queue.o(.text.*)
  *libcsi.a:k_pend.o(.text.*)
  *libcsi.a:k_tick.o(.text.*)
  *libcsi.a:k_time.o(.text.*)
  *libcsi.a:k_timer.o(.text.*)
  *libcsi.a:k_event.o(.text.krhino_event_get .text.krhino_event_set)
  *libcsi.a:k_mutex.o(.text.krhino_mutex_unlock .text.krhino_mutex_lock text.mutex_pri_look text.mutex_task_pri_reset)
  *libcsi.a:k_sem.o(.text*)
  *libcsi.a:k_sys.o(.text.krhino_intrpt_exit .text.krhino_intrpt_enter)
  *libcsi.a:k_task.o(.text.krhino_cur_task_get)
  *libcsi.a:port_s.o(.text*)
  *libcsi.a:hook_impl.o(.text.*)
  *libcsi.a:hook_weak.o(.text.*)
  *libcsi.a:systick.o(.text.*)

  /* lpm */
  *libcsi.a:lpm_arch_reg_save.o(.text.*)
  *libcsi.a:phy_pmu.o(.text.csi_pmu_enter_sleep)
  *libcsi.a:power_manager.o(.text.*)

  /* lib mm */
  *libcsi.a:mm_free.o(.text.*)
  *libcsi.a:mm_malloc.o(.text.*)
  *libcsi.a:mm_size2ndx.o(.text.*)
  *libcsi.a:mm_mallinfo.o(.text.*)
  *libcsi.a:mm_addfreechunk.o(.text.*)

  /* csi drivers */
  *libcsi.a:ck_irq.o(.text.*)
  *libcsi.a:isr.o(.text.*)
  *libcsi.a:common.o(.text.*)
  *libcsi.a:sys_freq.o(.text.*)
  *libcsi.a:dw_gpio.o(.text.dw_gpio_irqhandler .text.csi_gpio_pin_write .text.csi_gpio_pin_read)
  *libcsi.a:dw_usart.o(.text.csi_usart_receive_query .text.csi_usart_send .text.csi_usart_putchar .text.dw_usart_intr_recv_data .text.dw_usart_irqhandler .text.usart_wait_timeout*)
  *libcsi.a:dw_wdt.o(.text.dw_wdt_irqhandler)
  *libcsi.a:gpio_usart.o(.text.drv_gpio_usart_send_char .text.tx_gpio_write .text.delay_us)
  *libcsi.a:spif.o(.text.phy_wait_for_spif* .text.phy_init_spif .text.phy_enable_spif_flash_write .text.phy_disable_spif_flash_write .text.phy_check_internal_bus_busy .text.phy_wait_internal_bus_busy* .text.phy_ProgramPage64 .text.phy_ReadFlash .text.phy_flash_sector_erase)
  *pm.o(.text* .scommon COMMON)

  /* bt host */
  *libbt_host.a:aos_port.o(.text.irq_lock .text.irq_unlock .text.k_sem_take .text.k_sem_give .text.k_timer_start .text.k_timer_stop .text.k_queue_insert .text.k_queue_append .text.k_queue_prepend .text.k_queue_get)
  *libbt_host.a:atomic_c.o(.text.*)
  *libbt_host.a:buf.o(.text.*)
  *libbt_host.a:att.o(.text.bt_att_req_send .text.bt_att_send .text.att_read_req .text.att_handle_write_rsp .text.att_handle_read_rsp .text.att_handle_rsp .text.att_chan_get .text.att_cb* .text.att_req_sent .text.att_rsp_sent .text.att_get .text.check_perm .text.att_send_req .text.att_is_connected .text.att_cfm_sent .text.bt_att_recv .text.att_write_req .text.att_read_rsp .text.bt_att_create_pdu .text.read_cb .text.write_cb .text.att_confirm .text.att_pdu_sent .text.att_req_destroy .text.att_op_get_type .text.att_notify )
  *libbt_host.a:ble.o(.text.ble_stack_gatt_write .text.ble_stack_gatt_notify_cb .text.gatt_write_cb .text.gatt_read_handle .text.gatt_write_handle .text.ble_stack_event_callback* .text.ble_stack_gatt_notificate .text.ble_gatt_attr_notify)
  *libbt_host.a:conn.o(.text.bt_conn_recv .text.bt_conn_lookup_id .text.bt_conn_send_cb .text.create_frag* .text.bt_conn_lookup_addr_le bt_conn_addr_le_cmp .text.send_frag .text.bt_conn_reset_rx_state .text.bt_conn_get_id .text.bt_conn_unref .text.bt_conn_ref .text.bt_conn_lookup_handle .text.bt_conn_get_pkts .text.bt_conn_notify_tx .text.bt_conn_process_tx .text.bt_conn_create_pdu)
  *libbt_host.a:gatt.o(.text.bt_gatt_foreach_attr .text.bt_gatt_write .text.bt_gatt_write_without_response .text.bt_gatt_attr_read .text.gatt_send* .text.gatt_notify .text.gatt_write_rsp .text.find_next .text.bt_gatt_attr_next bt_gatt_notification .text.bt_gatt_indicate .text.bt_gatt_notify .text.notify_cb .text.gatt_indicate)
  *libbt_host.a:h4_driver.o(.text.h4_send .text.hci_event_recv .text.hci_acl_recv)
  *libbt_host.a:hci_core.o(.text.bt_hci_num_complete_packets .text.bt_recv .text.hci_acl .text.bt_send .text.bt_hci_conn_notify_tx .text.report_completed_packet)
  *libbt_host.a:l2cap.o(.text.bt_l2cap_recv .text.bt_l2cap_le_lookup_rx_cid .text.bt_l2cap_le_lookup_tx_cid .text.bt_l2cap_send_cb .text.bt_l2cap_create_pdu)
  *libbt_host.a:poll.o(.text.*)
  *libbt_host.a:work.o(.text.k_delayed_work_cancel .text.work_queue_thread .text.work_timeout .text.k_work_submit_to_queue .text.k_work_submit .text.k_delayed_work_submit)

  /* driver bt */
  *libdrivers_bt.a:ch6121_hci.o(.text.ch6121_hci_send .text.ble_stack_main .text.ch6121_hci_sync .text.control2host_send_message .rodata.tasksArr .rodata.tasksCnt)
  *libdrivers_bt.a:hci.o(.text.*)
  *libdrivers_bt.a:hci_api.o(.text.wait_llWaitingIrq* .text.hci_api_host_num_complete_packets .text.hci_api_num_complete_packets)
  *libdrivers_bt.a:hci_data.o(.text.*)
  *libdrivers_bt.a:hci_event.o(.text.*)
  *libdrivers_bt.a:hci_tl.o(.text.*)
  *libdrivers_bt.a:ll_patch.o(.text.*)
  *libdrivers_bt.a:simple_data.o(.text.*)
  *libdrivers_bt.a:stack_main.o(.text.*)

  /* board */
  *libboards_ch6121.a:pm.o(.text.enter_sleep_off_mode11 .text.config_RTC11 .text.enterSleepProcess11 .text.sys_soc_suspend .text.sys_soc_resume)

  /* kv */
  *libkv.a:block.o(.text.kvblock_search)

  /* lpm */
  *liblpm.a:lpm.o(.text.*)

  /* libs */
  *liblibs.a:ringbuffer.o(.text.ringbuffer_available_read_space .text.ringbuffer_write .text.ringbuffer_read)
  *liblibs.a:string.o(.text.memcmp .text.memcpy .text.memset)
  *liblibs.a:(.text.strdup .text.vsprintf)

  /* yoc kernel */
  *libyoc_kernel.a:kernel.o(.text.rhino2stderrno .text.aos_task_self .text.aos_mutex_lock .text.aos_mutex_unlock .text.aos_sem_new .text.aos_sem_free .text.aos_mutex_is_valid .text.aos_sem_wait .text.aos_sem_signal .text.aos_sem_is_valid .text.aos_event_get .text.aos_event_set .text.aos_queue_send .text.aos_queue_recv .text.aos_now_ms .text.aos_msleep .text.aos_kernel_sched_suspend .text.aos_kernel_sched_resume .text.aos_task_yield .text.aos_kernel_tick2ms .text.aos_kernel_ms2tick .text.aos_kernel_suspend .text.aos_kernel_resume)
  *libyoc_kernel.a:list.o(.text.*)
  *libyoc_kernel.a:malloc.o(.text.*)
  *libyoc_kernel.a:mm.o(.text.*)
  *libyoc_kernel.a:uservice.o(.text.*)
  *libyoc_kernel.a:rpc.o(.text.*)

  /* yoc drivers */
  *libdrivers.a:hci_drv.o(.text.hci_send)
  *libdrivers.a:uart_drv.o(.text.usart_csky_event_cb_fun .text.uart_csky_send .text.uart_csky_recv)
  *libdrivers.a:uart.o(.text.uart_recv .text.uart_send)
  *libdrivers.a:device.o(.text.device_lock .text.device_unlock .text.device_find .text.device_valid)

  /* at */
  *libat.a:atserver.o(.text.uart_event .text.interrupt_cmd_handle .text.atserver_rawdata_send .text.pass_through_handle .text.atserver_passthrough_cb_register .text.atserver_exit_passthrough)

  /* gcc lib */
  *libgcc.a:_thumb1_case_*.o(.text*)
  *libc.a:*strcmp.o(.text*)
  *libc.a:(.text.strstr .text.memmove .text.vsscanf)

  /* DO NOT MODIFY SECTIONS ABOVE!!!!!!!!! */

  /* ADD NEW SECTIONS HERE */

  /* applications */
  *libmain.a:init.o(.text.pm_prepare_sleep_action .text.pm_after_sleep_action .text.gpio_uart_init .text.set_sleep_pin_interrupt_mode .text.at_mode_pin_init*)
  *libmain.a:app_main.o(.text.uart_profile_recv .text.uartmode_recv .text.atmode_recv .text.at_mode_send .text.uart_mode_send .text.at_mode_ctrl)
  *libcsi.a:pinmux.o(.text.drv_pinmux_config .text.drv_pin_config_mode)
  *libcsi.a:gpio_usart.o(.text.*)
  *libcsi.a:device.o(.text.* .rodata*)
  *libcsi.a:dw_usart.o(.text.* .rodata*)
  *libdrivers.a:uart_drv.o(.text.uart_resume)
  *libcsi.a:clock.o(.text.clk_*)

  /* at ble */
  *libaters.a:at_ble.o(.text.at_ble_uartmode_recv  .text.at_evt_send)

  /* ble profile */
  *libble_profile.a:uart_client.o(.text.uart_client_send .text.uart_event_callback)
  *libble_profile.a:uart_server.o(.text.uart_server_send .text.uart_event_callback)

  /* fota */
  *libfota.a:flash.o(.text.flash_write)
  *libfota.a:serial.o(.text.serial_read .text.pass_th_cb)

  *gpio.o(.text*)
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
  . = ALIGN(0x4) ;
 } > REGION_NORETENTION0

}

