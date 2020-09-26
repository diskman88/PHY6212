/*******************************************************************************
 * IRQ_PRIORITY_ORDER
 *
 * Macro which should be defined as an integer of 0 or 1.
 *
 * This should be 0 if lower IRQ priority values implies higher priority
 * levels, such as on ARM Cortex M. If the opposite scheme is used, i.e.,
 * if higher IRQ priority values means higher priority, this should be 1.
 *
 * This setting is not critical. It is used only to sort and colorize the
 * interrupts in priority order, in case you record interrupts using
 * the trace_store_isr_begin and trace_store_isr_end routines.
 *
 ******************************************************************************
 *
 * HWTC Macros
 *
 * These four HWTC macros provides a hardware isolation layer representing a
 * generic hardware timer/counter used for driving the operating system tick,
 * such as the SysTick feature of ARM Cortex M3/M4, or the PIT of the Atmel
 * AT91SAM7X.
 *
 * HWTC_COUNT: The current value of the counter. This is expected to be reset
 * a each tick interrupt. Thus, when the tick handler starts, the counter has
 * already wrapped.
 *
 * HWTC_COUNT_DIRECTION: Should be one of:
 * - DIRECTION_INCREMENTING - for hardware timer/counters of incrementing type.
 *	When the counter value reach HWTC_PERIOD, it is reset to zero and the
 *	interrupt is signaled.
 * - DIRECTION_DECREMENTING - for hardware timer/counters of decrementing type.
 *	When the counter value reach 0, it is reset to HWTC_PERIOD and the
 *	interrupt is signaled.
 *
 * HWTC_PERIOD: The number of increments or decrements of HWTC_COUNT between
 * two OS tick interrupts. This should preferably be mapped to the reload
 * register of the hardware timer, to make it more portable between chips in the
 * same family. The macro should in most cases be (reload register + 1).
 * For FreeRTOS, this can in most cases be defined as
 * #define HWTC_PERIOD (configCPU_CLOCK_HZ / configTICK_RATE_HZ)
 *
 * HWTC_DIVISOR: If the timer frequency is very high, like on the Cortex M chips
 * (where the SysTick runs at the core clock frequency), the "differential
 * timestamping" used in the recorder will more frequently insert extra XTS
 * events to store the timestamps, which increases the event buffer usage.
 * In such cases, to reduce the number of XTS events and thereby get longer
 * traces, you use HWTC_DIVISOR to scale down the timestamps and frequency.
 * Assuming a OS tick rate of 1 KHz, it is suggested to keep the effective timer
 * frequency below 65 MHz to avoid an excessive amount of XTS events.
 *
 * The HWTC macros and trace_port_get_time_stamp is the main porting issue
 * or the trace recorder library. Typically you should not need to change
 * the code of trace_port_get_time_stamp if using the HWTC macros.
 *
 ******************************************************************************/

#include <k_config.h>
#if (SELECTED_PORT == PORT_CSKY_ABIV1)
    #define HWTC_COUNT_DIRECTION                DIRECTION_DECREMENTING
    /* FIXME: HWTC_COUNT should be timer current count value.  */
    #define HWTC_COUNT                          (drv_get_sys_freq()/RHINO_CONFIG_TICKS_PER_SECOND - 1)
    #define HWTC_PERIOD                         (drv_get_sys_freq()/RHINO_CONFIG_TICKS_PER_SECOND - 1)
	  #define HWTC_DIVISOR                        1
	  #define IRQ_PRIORITY_ORDER                  0
    #define HWTC_COUNT_FREQ                     drv_get_sys_freq()

#elif (SELECTED_PORT == PORT_CSKY_ABIV2)
    #define HWTC_COUNT_DIRECTION                DIRECTION_DECREMENTING /* FIXME: HWTC_COUNT should be timer current count value.  */
    #define HWTC_COUNT                          (drv_get_sys_freq()/RHINO_CONFIG_TICKS_PER_SECOND - 1)
    #define HWTC_PERIOD                         (drv_get_sys_freq()/RHINO_CONFIG_TICKS_PER_SECOND - 1)
	  #define HWTC_DIVISOR                        1
	  #define IRQ_PRIORITY_ORDER                  0
    #define HWTC_COUNT_FREQ                     drv_get_sys_freq()


#elif (SELECTED_PORT == PORT_APPLICATION_DEFINED)

	#if !( defined (HWTC_COUNT_DIRECTION) && defined (HWTC_COUNT) && defined (HWTC_PERIOD) && defined (HWTC_DIVISOR) && defined (IRQ_PRIORITY_ORDER) )
		#error SELECTED_PORT is PORT_APPLICATION_DEFINED but not all of the necessary constants have been defined.
	#endif

#elif (SELECTED_PORT != PORT_NOT_SET)

	#error "SELECTED_PORT had unsupported value!"
	#define SELECTED_PORT PORT_NOT_SET

#endif

