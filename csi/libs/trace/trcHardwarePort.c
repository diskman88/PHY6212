#include "trcHardwarePort.h"
#include "trcKernelPort.h"

#if (USE_TRACEALYZER_RECORDER == 1)

#include <stdint.h>

uint32_t trace_disable_timestamp = 0;
uint32_t last_timestamp = 0;

/*******************************************************************************
 * uiTraceTickCount
 *
 * This variable is should be updated by the Kernel tick interrupt. This does
 * not need to be modified when developing a new timer port. It is preferred to
 * keep any timer port changes in the HWTC macro definitions, which typically
 * give sufficient flexibility.
 ******************************************************************************/
uint32_t uiTraceTickCount = 0;

uint32_t DWT_CYCLES_ADDED = 0; /* Used on ARM Cortex-M only */

/******************************************************************************
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
void trace_port_get_time_stamp(uint32_t *pts)
{
	static uint32_t last_traceTickCount = 0;
	static uint32_t last_hwtc_count = 0;
	uint32_t traceTickCount = 0;
	uint32_t hwtc_count = 0;

	if (trace_disable_timestamp == 1)
	{
		if (pts)
			*pts = last_timestamp;
		return;
	}

	/* Retrieve HWTC_COUNT only once since the same value should be used all throughout this function. */
#if (HWTC_COUNT_DIRECTION == DIRECTION_INCREMENTING)
	hwtc_count = HWTC_COUNT;
#elif (HWTC_COUNT_DIRECTION == DIRECTION_DECREMENTING)
	hwtc_count = HWTC_PERIOD - HWTC_COUNT;
#else
	Junk text to cause compiler error - HWTC_COUNT_DIRECTION is not set correctly!
	Should be DIRECTION_INCREMENTING or DIRECTION_DECREMENTING
#endif

	if (last_traceTickCount - uiTraceTickCount - 1 < 0x80000000)
	{
		/* This means last_traceTickCount is higher than uiTraceTickCount,
		so we have previously compensated for a missed tick.
		Therefore we use the last stored value because that is more accurate. */
		traceTickCount = last_traceTickCount;
	}
	else
	{
		/* Business as usual */
		traceTickCount = uiTraceTickCount;
	}

	/* Check for overflow. May occur if the update of uiTraceTickCount has been
	delayed due to disabled interrupts. */
	if (traceTickCount == last_traceTickCount && hwtc_count < last_hwtc_count)
	{
		/* A trace tick has occurred but not been executed by the kernel, so we compensate manually. */
		traceTickCount++;
	}

	/* Check if the return address is OK, then we perform the calculation. */
	if (pts)
	{
		/* Get timestamp from trace ticks. Scale down the period to avoid unwanted overflows. */
		*pts = traceTickCount * (HWTC_PERIOD / HWTC_DIVISOR);
		/* Increase timestamp by (hwtc_count + "lost hardware ticks from scaling down period") / HWTC_DIVISOR. */
		*pts += (hwtc_count + traceTickCount * (HWTC_PERIOD % HWTC_DIVISOR)) / HWTC_DIVISOR;

		last_timestamp = *pts;
	}

	/* Store the previous values. */
	last_traceTickCount = traceTickCount;
	last_hwtc_count = hwtc_count;
}

#endif
