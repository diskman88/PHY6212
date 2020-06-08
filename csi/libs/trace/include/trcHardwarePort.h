#ifndef __TRCPORT_H__
#define __TRCPORT_H__
#include <stdint.h>
#include <csi_config.h>
#include <soc.h>

#define DIRECTION_INCREMENTING 1
#define DIRECTION_DECREMENTING 2

/******************************************************************************
 * Supported ports
 *
 * PORT_HWIndependent
 * A hardware independent fallback option for event timestamping. Provides low
 * resolution timestamps based on the OS tick.
 *
 * PORT_APPLICATION_DEFINED
 * Allows for defining the port macros in other source code files.
 *
 * Hardware specific ports
 * To get accurate timestamping, a hardware timer is necessary. Below are the
 * available ports. Some of these are "unofficial", meaning that
 * they have not yet been verified by Percepio but have been contributed by
 * external developers. They should work, otherwise let us know by emailing
 * support@percepio.com. Some work on any OS platform, while other are specific
 * to a certain operating system.
 *****************************************************************************/
/****** Port Name ********************** Code ***** Official ** OS Platform *********/
#define PORT_CSKY_ABIV1	    				1	/*	Yes			Any					*/
#define PORT_CSKY_ABIV2	    				2	/*	Yes			Any					*/



#include "trcConfig.h"

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
#ifdef CONFIG_KERNEL_UCOS
#include "trcHardwarePort_ucos.h"
#elif CONFIG_KERNEL_FREERTOS
#include "trcHardwarePort_freertos.h"
#elif CONFIG_KERNEL_RHINO
#include "trcHardwarePort_rhino.h"
#endif

#if (SELECTED_PORT != PORT_NOT_SET)

	#ifndef HWTC_COUNT_DIRECTION
	#error "HWTC_COUNT_DIRECTION is not set!"
	#endif

	#ifndef HWTC_COUNT
	#error "HWTC_COUNT is not set!"
	#endif

	#ifndef HWTC_PERIOD
	#error "HWTC_PERIOD is not set!"
	#endif

	#ifndef HWTC_DIVISOR
	#error "HWTC_DIVISOR is not set!"
	#endif

	#ifndef IRQ_PRIORITY_ORDER
	#error "IRQ_PRIORITY_ORDER is not set!"
	#elif (IRQ_PRIORITY_ORDER != 0) && (IRQ_PRIORITY_ORDER != 1)
	#error "IRQ_PRIORITY_ORDER has bad value!"
	#endif

	#if (HWTC_DIVISOR < 1)
	#error "HWTC_DIVISOR must be a non-zero positive value!"
	#endif

#endif
/*******************************************************************************
 * vTraceConsoleMessage
 *
 * A wrapper for your system-specific console "printf" console output function.
 * This needs to be correctly defined to see status reports from the trace
 * status monitor task (this is defined in trcUser.c).
 ******************************************************************************/
#define vTraceConsoleMessage(x)

/*******************************************************************************
 * trace_port_get_time_stamp
 *
 * Returns the current time based on the HWTC macros which provide a hardware
 * isolation layer towards the hardware timer/counter.
 *
 * The HWTC macros and trace_port_get_time_stamp is the main porting issue
 * or the trace recorder library. Typically you should not need to change
 * the code of trace_port_get_time_stamp if using the HWTC macros.
 *
 ******************************************************************************/
void trace_port_get_time_stamp(uint32_t *pts);


#endif
