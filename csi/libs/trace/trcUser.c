#include "trcUser.h"

#if (USE_TRACEALYZER_RECORDER == 1)

#include <string.h>
#include <stdarg.h>
#include <stdint.h>

TRACE_STOP_HOOK trace_stopHookPtr = (TRACE_STOP_HOOK)0;

extern uint8_t inExcludedTask;
extern int8_t nISRactive;
extern object_handle_t handle_of_last_logged_task;
extern uint32_t dts_min;
extern uint32_t hwtc_count_max_after_tick;
extern uint32_t hwtc_count_sum_after_tick;
extern uint32_t hwtc_count_sum_after_tick_counter;
extern char* traceErrorMessage;

/*** Private functions *******************************************************/
void trace_printf_Helper(traceLabel eventLabel, const char* formatStr, va_list vl);

#if (USE_SEPARATE_USER_EVENT_BUFFER == 1)
void trace_channel_printf_Helper(UserEventChannel channelPair, va_list vl);
static void prtrace_user_eventHelper1(UserEventChannel channel, traceLabel eventLabel, traceLabel formatLabel, va_list vl);
static void prtrace_user_eventHelper2(UserEventChannel channel, uint32_t* data, uint32_t noOfSlots);
#endif

static void prtrace_task_instance_finish(int8_t direct);


/*******************************************************************************
 * trace_init_data
 *
 * Allocates, if necessary, and initializes the recorder data structure, based
 * on the constants in trcConfig.h.
 ******************************************************************************/
void trace_init_data(void)
{
	trace_init_trace_data();
}

/*******************************************************************************
 * trace_set_recorder_data
 *
 * If custom allocation is used, this function must be called so the recorder
 * library knows where to save the trace data.
 ******************************************************************************/
#if TRACE_DATA_ALLOCATION == TRACE_DATA_ALLOCATION_CUSTOM
void trace_set_recorder_data(void* pRecorderData)
{
	TRACE_ASSERT(pRecorderData != NULL, "vTraceSetTraceData, pRecorderData == NULL", );
	g_recorder_data_ptr = pRecorderData;
}
#endif

/*******************************************************************************
 * trace_set_stop_hook
 *
 * Sets a function to be called when the recorder is stopped.
 ******************************************************************************/
void trace_set_stop_hook(TRACE_STOP_HOOK stopHookFunction)
{
	trace_stopHookPtr = stopHookFunction;
}

/*******************************************************************************
 * trace_clear
 *
 * Resets the recorder. Only necessary if a restart is desired - this is not
 * needed in the startup initialization.
 ******************************************************************************/
void trace_clear(void)
{
	TRACE_SR_ALLOC_CRITICAL_SECTION();
	trcCRITICAL_SECTION_BEGIN();

	g_recorder_data_ptr->abs_time_last_event_second = 0;

	g_recorder_data_ptr->abs_time_last_event = 0;
	g_recorder_data_ptr->next_index = 0;
	g_recorder_data_ptr->nevents = 0;
	g_recorder_data_ptr->is_buffer_full = 0;
	traceErrorMessage = NULL;
	g_recorder_data_ptr->internal_error = 0;

	memset(g_recorder_data_ptr->event_data, 0, g_recorder_data_ptr->max_events * 4);

	handle_of_last_logged_task = 0;
	
	trcCRITICAL_SECTION_END();

}

/*******************************************************************************
 * tarce_start_internal
 *
 * Starts the recorder. The recorder will not be started if an error has been
 * indicated using trace_error, e.g. if any of the Nx constants in trcConfig.h
 * has a too small value (NTASK, NQUEUE, etc).
 *
 * Returns 1 if the recorder was started successfully.
 * Returns 0 if the recorder start was prevented due to a previous internal
 * error. In that case, check vTraceGetLastError to get the error message.
 * Any error message is also presented when opening a trace file.
 ******************************************************************************/

uint32_t tarce_start_internal(void)
{
	object_handle_t handle;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	handle = 0;

	if (g_recorder_data_ptr == NULL)
	{
		trace_error("g_recorder_data_ptr is NULL. Call trace_init_data() before starting trace.");
		return 0;
	}

	if (traceErrorMessage == NULL)
	{
		trcCRITICAL_SECTION_BEGIN();
		g_recorder_data_ptr->recorder_actived = 1;

		handle = TRACE_GET_TASK_NUMBER(TRACE_GET_CURRENT_TASK());
		if (handle == 0)
		{
			/* This occurs if the scheduler is not yet started.
			This creates a dummy "(startup)" task entry internally in the
			recorder */
			handle = trace_get_object_handle(TRACE_CLASS_TASK);
			trace_set_object_name(TRACE_CLASS_TASK, handle, "(startup)");

			trace_set_priority_property(TRACE_CLASS_TASK, handle, 0);
		}

		trace_store_taskswitch(handle); /* Register the currently running task */
		trcCRITICAL_SECTION_END();
	}

	return g_recorder_data_ptr->recorder_actived;
}

/*******************************************************************************
 * trace_start
 *
 * Starts the recorder. The recorder will not be started if an error has been
 * indicated using trace_error, e.g. if any of the Nx constants in trcConfig.h
 * has a too small value (NTASK, NQUEUE, etc).
 *
 * This function is obsolete, but has been saved for backwards compatibility.
 * We recommend using tarce_start_internal instead.
 ******************************************************************************/
void trace_start(void)
{
	(void)tarce_start_internal();
}

/*******************************************************************************
 * trace_stop
 *
 * Stops the recorder. The recording can be resumed by calling trace_start.
 * This does not reset the recorder. Use trace_clear if that is desired.
 ******************************************************************************/
void trace_stop(void)
{
	g_recorder_data_ptr->recorder_actived = 0;

	if (trace_stopHookPtr != (TRACE_STOP_HOOK)0)
	{
		(*trace_stopHookPtr)();			/* An application call-back function. */
	}
}

/*******************************************************************************
 * trace_get_last_error
 *
 * Gives the last error message, if any. NULL if no error message is stored.
 * Any error message is also presented when opening a trace file.
 ******************************************************************************/
char* trace_get_last_error(void)
{
	return traceErrorMessage;
}

/*******************************************************************************
* trace_clear_error
*
* Removes any previous error message generated by recorder calling trace_error.
* By calling this function, it may be possible to start/restart the trace
* despite errors in the recorder, but there is no guarantee that the trace
* recorder will work correctly in that case, depending on the type of error.
******************************************************************************/
void trace_clear_error(int resetErrorMessage)
{
	( void ) resetErrorMessage;
	traceErrorMessage = NULL;
	g_recorder_data_ptr->internal_error = 0;
}

/*******************************************************************************
 * trace_get_trace_buffer
 *
 * Returns a pointer to the recorder data structure. Use this together with
 * trace_get_trace_buffer_size if you wish to implement an own store/upload
 * solution, e.g., in case a debugger connection is not available for uploading
 * the data.
 ******************************************************************************/
void* trace_get_trace_buffer(void)
{
	return g_recorder_data_ptr;
}

/*******************************************************************************
 * trace_get_trace_buffer_size
 *
 * Gets the size of the recorder data structure. For use together with
 * trace_get_trace_buffer if you wish to implement an own store/upload solution,
 * e.g., in case a debugger connection is not available for uploading the data.
 ******************************************************************************/
uint32_t trace_get_trace_buffer_size(void)
{
	return sizeof(recorder_data_t);
}

/******************************************************************************
 * prtrace_task_instance_finish.
 *
 * Private common function for the trace_task_instance_finishXXX functions.
 * 
 *****************************************************************************/
void prtrace_task_instance_finish(int8_t direct)
{
	TaskInstanceStatusEvent* tis;
	uint8_t dts45;

	TRACE_SR_ALLOC_CRITICAL_SECTION();

	trcCRITICAL_SECTION_BEGIN();
	if (g_recorder_data_ptr->recorder_actived && (! inExcludedTask || nISRactive) && handle_of_last_logged_task)
	{
		dts45 = (uint8_t)trace_get_dts(0xFF);
		tis = (TaskInstanceStatusEvent*) trace_next_free_event_buffer_slot();
		if (tis != NULL)
		{
			if (direct == 0)
				tis->type = TASK_INSTANCE_FINISHED_NEXT_KSE;
			else
				tis->type = TASK_INSTANCE_FINISHED_DIRECT;

			tis->dts = dts45;
			trace_update_counters();
		}
	}
	trcCRITICAL_SECTION_END();
}

/******************************************************************************
 * trace_task_instance_finish(void)
 *
 * Marks the current task instance as finished on the next kernel call.
 *
 * If that kernel call is blocking, the instance ends after the blocking event
 * and the corresponding return event is then the start of the next instance.
 * If the kernel call is not blocking, the viewer instead splits the current
 * fragment right before the kernel call, which makes this call the first event
 * of the next instance.
 *
 * See also USE_IMPLICIT_IFE_RULES in trcConfig.h
 *
 * Example:
 *
 *		while(1)
 *		{
 *			xQueueReceive(CommandQueue, &command, timeoutDuration);
 *			processCommand(command);
 *          vTraceInstanceFinish();
 *		}
 *
 * Note: This is only supported in Tracealyzer tools v2.7 or later
 *
 *****************************************************************************/
void trace_task_instance_finish(void)
{
    prtrace_task_instance_finish(0);
}

/******************************************************************************
 * trace_task_instance_finish_direct(void)
 *
 * Marks the current task instance as finished at this very instant.
 * This makes the viewer to splits the current fragment at this point and begin
 * a new actor instance.
 *
 * See also USE_IMPLICIT_IFE_RULES in trcConfig.h
 *
 * Example:
 *
 *		This example will generate two instances for each loop iteration.
 *		The first instance ends at trace_task_instance_finish_direct(), while the second
 *      instance ends at the next xQueueReceive call.
 *
 *		while (1)
 *		{
 *          xQueueReceive(CommandQueue, &command, timeoutDuration);
 *			ProcessCommand(command);
 *			trace_task_instance_finish_direct();
 *			DoSometingElse();
 *          vTraceInstanceFinish();
 *      }
 *
 * Note: This is only supported in Tracealyzer tools v2.7 or later
 *
 *****************************************************************************/
void trace_task_instance_finish_direct(void)
{
	prtrace_task_instance_finish(1);
}

/*******************************************************************************
 * Interrupt recording functions
 ******************************************************************************/

#if (INCLUDE_ISR_TRACING == 1)

#define MAX_ISR_NESTING 16
static uint8_t isrstack[MAX_ISR_NESTING];
int32_t isPendingContextSwitch = 0;

/*******************************************************************************
 * trace_set_isr_properties
 *
 * Registers an Interrupt Service Routine in the recorder library, This must be
 * called before using trace_store_isr_begin to store ISR events. This is
 * typically called in the startup of the system, before the scheduler is
 * started.
 *
 * Example:
 *	 #define ID_ISR_TIMER1 1		// lowest valid ID is 1
 *	 #define PRIO_OF_ISR_TIMER1 3 // the hardware priority of the interrupt
 *	 ...
 *	 trace_set_isr_properties(ID_ISR_TIMER1, "ISRTimer1", PRIO_OF_ISR_TIMER1);
 *	 ...
 *	 void ISR_handler()
 *	 {
 *		 trace_store_isr_begin(ID_OF_ISR_TIMER1);
 *		 ...
 *		 trace_store_isr_end(0);
 *	 }
 *
 * NOTE: To safely record ISRs, you need to make sure that all traced
 * interrupts actually are disabled by trcCRITICAL_SECTION_BEGIN(). However,
 * in some ports this does not disable high priority interrupts!
 * If an ISR calls trace_store_isr_begin while the recorder is busy, it will
 * stop the recording and give an error message.
 ******************************************************************************/
void trace_set_isr_properties(object_handle_t handle, const char* name, char priority)
{
	TRACE_ASSERT(handle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[TRACE_CLASS_ISR], "trace_set_isr_properties: Invalid value for handle", );
	TRACE_ASSERT(name != NULL, "trace_set_isr_properties: name == NULL", );

	trace_set_object_name(TRACE_CLASS_ISR, handle, name);
	trace_set_priority_property(TRACE_CLASS_ISR, handle, priority);
}

/*******************************************************************************
 * trace_store_isr_begin
 *
 * Registers the beginning of an Interrupt Service Routine.
 *
 * Example:
 *	 #define ID_ISR_TIMER1 1		// lowest valid ID is 1
 *	 #define PRIO_OF_ISR_TIMER1 3 // the hardware priority of the interrupt
 *	 ...
 *	 trace_set_isr_properties(ID_ISR_TIMER1, "ISRTimer1", PRIO_OF_ISR_TIMER1);
 *	 ...
 *	 void ISR_handler()
 *	 {
 *		 trace_store_isr_begin(ID_OF_ISR_TIMER1);
 *		 ...
 *		 trace_store_isr_end(0);
 *	 }
 *
 ******************************************************************************/
void trace_store_isr_begin(object_handle_t handle)
{
	uint16_t dts4;
	TSEvent* ts;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	ts = NULL;

	if (recorder_busy)
	{
	 trace_error("Illegal call to trace_store_isr_begin, recorder busy!");
	 return;
	}
	trcCRITICAL_SECTION_BEGIN();
	
	if (nISRactive == 0)
		isPendingContextSwitch = 0;	/* We are at the start of a possible ISR chain. No context switches should have been triggered now. */
	
	if (g_recorder_data_ptr->recorder_actived && handle_of_last_logged_task)
	{

		TRACE_ASSERT(handle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[TRACE_CLASS_ISR], "trace_store_isr_begin: Invalid value for handle", );
		
		dts4 = (uint16_t)trace_get_dts(0xFFFF);

		if (g_recorder_data_ptr->recorder_actived) /* Need to repeat this check! */
		{
			if (nISRactive < MAX_ISR_NESTING)
			{
				uint8_t hnd8 = trace_get_8bit_handle(handle);
				isrstack[nISRactive] = handle;
				nISRactive++;
				ts = (TSEvent*)trace_next_free_event_buffer_slot();
				if (ts != NULL)
				{
					ts->type = TS_ISR_BEGIN;
					ts->dts = dts4;
					ts->objHandle = hnd8;
					trace_update_counters();
				}
			}
			else
			{
				/* This should not occur unless something is very wrong */
				trace_error("Too many nested interrupts!");
			}
		}
	}
	trcCRITICAL_SECTION_END();
}

/*******************************************************************************
 * trace_store_isr_end
 *
 * Registers the end of an Interrupt Service Routine.
 *
 * The parameter pendingISR indicates if the interrupt has requested a
 * task-switch (= 1) or if the interrupt returns to the earlier context (= 0)
 *
 * Example:
 *
 *	 #define ID_ISR_TIMER1 1		// lowest valid ID is 1
 *	 #define PRIO_OF_ISR_TIMER1 3 // the hardware priority of the interrupt
 *	 ...
 *	 trace_set_isr_properties(ID_ISR_TIMER1, "ISRTimer1", PRIO_OF_ISR_TIMER1);
 *	 ...
 *	 void ISR_handler()
 *	 {
 *		 trace_store_isr_begin(ID_OF_ISR_TIMER1);
 *		 ...
 *		 trace_store_isr_end(0);
 *	 }
 *
 ******************************************************************************/
void trace_store_isr_end(int pendingISR)
{
	TSEvent* ts;
	uint16_t dts5;
	uint8_t hnd8 = 0, type = 0;
	
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	if (! g_recorder_data_ptr->recorder_actived ||  ! handle_of_last_logged_task)
	{
		return;
	}

	if (recorder_busy)
	{
		trace_error("Illegal call to trace_store_isr_end, recorder busy!");
		return;
	}
	
	if (nISRactive == 0)
	{
		trace_error("Unmatched call to trace_store_isr_end (nISRactive == 0, expected > 0)");
		return;
	}

	trcCRITICAL_SECTION_BEGIN();
	isPendingContextSwitch |= pendingISR;	/* Is there a pending context switch right now? */
	nISRactive--;
	if (nISRactive > 0)
	{
		/* Return to another isr */
		type = TS_ISR_RESUME;
		hnd8 = trace_get_8bit_handle(isrstack[nISRactive - 1]); /* isrstack[nISRactive] is the handle of the ISR we're currently exiting. isrstack[nISRactive - 1] is the handle of the ISR that was executing previously. */
	}
	else if (isPendingContextSwitch == 0)
	{
		/* No context switch has been triggered by any ISR in the chain. Return to task */
		type = TS_TASK_RESUME;
		hnd8 = trace_get_8bit_handle(handle_of_last_logged_task);
	}
	else
	{
		/* Context switch has been triggered by some ISR. We expect a proper context switch event shortly so we do nothing. */
	}

	if (type != 0)
	{
		dts5 = (uint16_t)trace_get_dts(0xFFFF);
		ts = (TSEvent*)trace_next_free_event_buffer_slot();
		if (ts != NULL)
		{
			ts->type = type;
			ts->objHandle = hnd8;
			ts->dts = dts5;
			trace_update_counters();
		}

	}

	trcCRITICAL_SECTION_END();
}

#else

/* ISR tracing is turned off */
void trace_increase_isr_active(void)
{
	if (g_recorder_data_ptr->recorder_actived && handle_of_last_logged_task)
		nISRactive++;
}

void trace_decrease_isr_active(void)
{
	if (g_recorder_data_ptr->recorder_actived && handle_of_last_logged_task)
		nISRactive--;
}
#endif


/********************************************************************************/
/* User Event functions															*/
/********************************************************************************/

#if (INCLUDE_USER_EVENTS == 1)

#define MAX_ARG_SIZE (4+32)
/*** Locally used in trace_printf ***/
static uint8_t writeInt8(void * buffer, uint8_t i, uint8_t value)
{
	TRACE_ASSERT(buffer != NULL, "writeInt8: buffer == NULL", 0);

	if (i >= MAX_ARG_SIZE)
	{
		return 255;
	}

	((uint8_t*)buffer)[i] = value;

	if (i + 1 > MAX_ARG_SIZE)
	{
		return 255;
	}

	return i + 1;
}

/*** Locally used in trace_printf ***/
static uint8_t writeInt16(void * buffer, uint8_t i, uint16_t value)
{
	TRACE_ASSERT(buffer != NULL, "writeInt16: buffer == NULL", 0);

	/* Align to multiple of 2 */
	while ((i % 2) != 0)
	{
		if (i >= MAX_ARG_SIZE)
		{
			return 255;
		}

		((uint8_t*)buffer)[i] = 0;
		i++;
	}

	if (i + 2 > MAX_ARG_SIZE)
	{
		return 255;
	}

	((uint16_t*)buffer)[i/2] = value;

	return i + 2;
}

/*** Locally used in trace_printf ***/
static uint8_t writeInt32(void * buffer, uint8_t i, uint32_t value)
{
	TRACE_ASSERT(buffer != NULL, "writeInt32: buffer == NULL", 0);

	/* A 32 bit value should begin at an even 4-byte address */
	while ((i % 4) != 0)
	{
		if (i >= MAX_ARG_SIZE)
		{
			return 255;
		}

		((uint8_t*)buffer)[i] = 0;
		i++;
	}

	if (i + 4 > MAX_ARG_SIZE)
	{
		return 255;
	}

	((uint32_t*)buffer)[i/4] = value;

	return i + 4;
}

#if (INCLUDE_FLOAT_SUPPORT)

/*** Locally used in trace_printf ***/
static uint8_t writeFloat(void * buffer, uint8_t i, float value)
{
	TRACE_ASSERT(buffer != NULL, "writeFloat: buffer == NULL", 0);

	/* A 32 bit value should begin at an even 4-byte address */
	while ((i % 4) != 0)
	{
		if (i >= MAX_ARG_SIZE)
		{
			return 255;
		}

		((uint8_t*)buffer)[i] = 0;
		i++;
	}

	if (i + 4 > MAX_ARG_SIZE)
	{
		return 255;
	}

	((float*)buffer)[i/4] = value;

	return i + 4;
}

/*** Locally used in trace_printf ***/
static uint8_t writeDouble(void * buffer, uint8_t i, double value)
{
	uint32_t * dest;
	uint32_t * src = (uint32_t*)&value;

	TRACE_ASSERT(buffer != NULL, "writeDouble: buffer == NULL", 0);

	/* The double is written as two 32 bit values, and should begin at an even
	4-byte address (to avoid having to align with 8 byte) */
	while (i % 4 != 0)
	{
		if (i >= MAX_ARG_SIZE)
		{
			return 255;
		}

		((uint8_t*)buffer)[i] = 0;
		i++;
	}

	if (i + 8 > MAX_ARG_SIZE)
	{
		return 255;
	}

	dest = &(((uint32_t *)buffer)[i/4]);

	dest[0] = src[0];
	dest[1] = src[1];

	return i + 8;
}

#endif

/*******************************************************************************
 * prtrace_user_eventFormat
 *
 * Parses the format string and stores the arguments in the buffer.
 ******************************************************************************/
static uint8_t prtrace_user_eventFormat(const char* formatStr, va_list vl, uint8_t* buffer, uint8_t byteOffset)
{
	uint16_t formatStrIndex = 0;
	uint8_t argCounter = 0;
	uint8_t i = byteOffset;

	while (formatStr[formatStrIndex] != '\0')
	{
		if (formatStr[formatStrIndex] == '%')
		{
			argCounter++;

			if (argCounter > 15)
			{
				trace_error("trace_printf - Too many arguments, max 15 allowed!");
				return 0;
			}

			/*******************************************************************************
			* These below code writes raw data (primitive datatypes) in the event buffer,
			* instead of the normal event structs (where byte 0 is event type).
			* These data entries must never be interpreted as real event data, as the type
			* field would be misleading since used for payload data.
			*
			* The correctness of this encoding depends on two mechanisms:
			*
			* 1. An initial USER_EVENT, which type code tells the number of 32-bit data
			* entires that follows. (code - USER_EVENT = number of data entries).
			* Note that a data entry corresponds to the slots that normally corresponds to
			* one (1) event, i.e., 32 bits. trace_printf may encode several pieces of data
			* in one data entry, e.g., two 16-bit values or four 8-bit values, one 16-bit
			* value followed by two 8-bit values, etc.
			*
			* 2. A two-phase commit procedure, where the USER_EVENT and data entries are
			* written to a local buffer at first, and when all checks are OK then copied to
			* the main event buffer using a fast memcpy. The event code is finalized as the
			* very last step. Before that step, the event code indicates an unfinished
			* event, which causes it to be ignored and stop the loading of the file (since
			* an unfinished event is the last event in the trace).
			*******************************************************************************/
			formatStrIndex++;

			while ((formatStr[formatStrIndex] >= '0' && formatStr[formatStrIndex] <= '9') || formatStr[formatStrIndex] == '#' || formatStr[formatStrIndex] == '.')
				formatStrIndex++;

			if (formatStr[formatStrIndex] != '\0')
			{
				switch (formatStr[formatStrIndex])
				{
					case 'd':	i = writeInt32(	buffer,
												i,
												(uint32_t)va_arg(vl, uint32_t));
								break;
					case 'x':
					case 'X':
					case 'u':	i = writeInt32(	buffer,
												i,
												(uint32_t)va_arg(vl, uint32_t));
								break;
					case 's':	i = writeInt16(	buffer,
												i,
												(uint16_t)trace_open_label((char*)va_arg(vl, char*)));
								break;

#if (INCLUDE_FLOAT_SUPPORT)
					/* Yes, "double" as type also in the float
					case. This since "float" is promoted into "double"
					by the va_arg stuff. */
					case 'f':	i = writeFloat(	buffer,
												i,
												(float)va_arg(vl, double));
								break;
#else
					/* No support for floats, but attempt to store a float user event
					avoid a possible crash due to float reference. Instead store the
					data on uint_32 format (will not be displayed anyway). This is just
					to keep va_arg and i consistent. */

					case 'f':	i = writeInt32(	buffer,
												i,
												(uint32_t)va_arg(vl, double));
								break;
#endif
					case 'l':
								formatStrIndex++;
								switch (formatStr[formatStrIndex])
								{
#if (INCLUDE_FLOAT_SUPPORT)
									case 'f':	i = writeDouble(buffer,
																i,
																(double)va_arg(vl, double));
												break;
#else
									/* No support for floats, but attempt to store a float user event
									avoid a possible crash due to float reference. Instead store the
									data on uint_32 format (will not be displayed anyway). This is just
									to keep va_arg and i consistent. */
									case 'f':	i = writeInt32(	buffer, /* In this case, the value will not be shown anyway */
																i,
																(uint32_t)va_arg(vl, double));

												i = writeInt32(	buffer, /* Do it twice, to write in total 8 bytes */
																i,
																(uint32_t)va_arg(vl, double));
										break;
#endif

								}
								break;
					case 'h':
								formatStrIndex++;
								switch (formatStr[formatStrIndex])
								{
									case 'd':	i = writeInt16(	buffer,
																i,
																(uint16_t)va_arg(vl, uint32_t));
												break;
									case 'u':	i = writeInt16(	buffer,
																i,
																(uint16_t)va_arg(vl, uint32_t));
												break;
								}
								break;
					case 'b':
								formatStrIndex++;
								switch (formatStr[formatStrIndex])
								{
									case 'd':	i = writeInt8(	buffer,
																i,
																(uint8_t)va_arg(vl, uint32_t));
												break;

									case 'u':	i = writeInt8(	buffer,
																i,
																(uint8_t)va_arg(vl, uint32_t));
												break;
								}
								break;
				}
			}
			else
				break;
		}
		formatStrIndex++;
		if (i == 255)
		{
			trace_error("trace_printf - Too large arguments, max 32 byte allowed!");
			return 0;
		}
	}
	return (i+3)/4;
}

#if (USE_SEPARATE_USER_EVENT_BUFFER == 1)

/*******************************************************************************
 * trace_clear_channel_buffer
 *
 * Clears a number of items in the channel buffer, starting from next_slot.
 ******************************************************************************/
static void trace_clear_channel_buffer(uint32_t count)
{
	uint32_t slots;

	TRACE_ASSERT(USER_EVENT_BUFFER_SIZE >= count,
		"trace_clear_channel_buffer: USER_EVENT_BUFFER_SIZE is too small to handle this event.", );

	/* Check if we're close to the end of the buffer */
	if (g_recorder_data_ptr->user_event_buffer.next_slot + count > USER_EVENT_BUFFER_SIZE)
	{
		slots = USER_EVENT_BUFFER_SIZE - g_recorder_data_ptr->user_event_buffer.next_slot; /* Number of slots before end of buffer */
		(void)memset(&g_recorder_data_ptr->user_event_buffer.channel_buffer[g_recorder_data_ptr->user_event_buffer.next_slot], 0, slots);
		(void)memset(&g_recorder_data_ptr->user_event_buffer.channel_buffer[0], 0, (count - slots));
	}
	else
		(void)memset(&g_recorder_data_ptr->user_event_buffer.channel_buffer[g_recorder_data_ptr->user_event_buffer.next_slot], 0, count);
}

/*******************************************************************************
 * prvTraceCopyToDataBuffer
 *
 * Copies a number of items to the data buffer, starting from next_slot.
 ******************************************************************************/
static void prvTraceCopyToDataBuffer(uint32_t* data, uint32_t count)
{
	TRACE_ASSERT(data != NULL,
		"prvTraceCopyToDataBuffer: data == NULL.", );
	TRACE_ASSERT(count <= USER_EVENT_BUFFER_SIZE,
		"prvTraceCopyToDataBuffer: USER_EVENT_BUFFER_SIZE is too small to handle this event.", );

	uint32_t slots;
	/* Check if we're close to the end of the buffer */
	if (g_recorder_data_ptr->user_event_buffer.next_slot + count > USER_EVENT_BUFFER_SIZE)
	{
		slots = USER_EVENT_BUFFER_SIZE - g_recorder_data_ptr->user_event_buffer.next_slot; /* Number of slots before end of buffer */
		(void)memcpy(&g_recorder_data_ptr->user_event_buffer.data_buffer[g_recorder_data_ptr->user_event_buffer.next_slot * 4], data, slots * 4);
		(void)memcpy(&g_recorder_data_ptr->user_event_buffer.data_buffer[0], data + slots, (count - slots) * 4);
	}
	else
	{
		(void)memcpy(&g_recorder_data_ptr->user_event_buffer.data_buffer[g_recorder_data_ptr->user_event_buffer.next_slot * 4], data, count * 4);
	}
}

/*******************************************************************************
 * prtrace_user_eventHelper1
 *
 * Calls on prtrace_user_eventFormat() to do the actual formatting, then goes on
 * to the next helper function.
 ******************************************************************************/
static void prtrace_user_eventHelper1(UserEventChannel channel, traceLabel eventLabel, traceLabel formatLabel, va_list vl)
{
	uint32_t data[(3 + MAX_ARG_SIZE) / 4];
	uint8_t byteOffset = 4; /* Need room for timestamp */
	uint8_t noOfSlots;

	if (channel == 0)
	{
		/* We are dealing with an unknown channel format pair */
		byteOffset += 4; /* Also need room for channel and format */
		((uint16_t*)data)[2] = eventLabel;
		((uint16_t*)data)[3] = formatLabel;
	}

	noOfSlots = prtrace_user_eventFormat((char*)&(g_recorder_data_ptr->symbol_table.symbytes[formatLabel+4]), vl, (uint8_t*)data, byteOffset);

	prtrace_user_eventHelper2(channel, data, noOfSlots);
}

/*******************************************************************************
 * prtrace_user_eventHelper2
 *
 * This function simply copies the data buffer to the actual user event buffer.
 ******************************************************************************/
static void prtrace_user_eventHelper2(UserEventChannel channel, uint32_t* data, uint32_t noOfSlots)
{
	static uint32_t old_timestamp = 0;
	uint32_t old_next_slot = 0;

	TRACE_ASSERT(USER_EVENT_BUFFER_SIZE >= noOfSlots, "trace_printf: USER_EVENT_BUFFER_SIZE is too small to handle this event.", );

	trcCRITICAL_SECTION_BEGIN();
	/* Store the timestamp */
	trace_port_get_time_stamp(data);

	if (*data < old_timestamp)
	{
		g_recorder_data_ptr->user_event_buffer.wraparound_counter++;
	}

	old_timestamp = *data;

	/* Start by erasing any information in the channel buffer */
	trace_clear_channel_buffer(noOfSlots);

	prvTraceCopyToDataBuffer(data, noOfSlots); /* Will wrap around the data if necessary */

	old_next_slot = g_recorder_data_ptr->user_event_buffer.next_slot; /* Save the index that we want to write the channel data at when we're done */
	g_recorder_data_ptr->user_event_buffer.next_slot = (g_recorder_data_ptr->user_event_buffer.next_slot + noOfSlots) % USER_EVENT_BUFFER_SIZE; /* Make sure we never end up outside the buffer */

	/* Write to the channel buffer to indicate that this user event is ready to be used */
	if (channel != 0)
	{
		g_recorder_data_ptr->user_event_buffer.channel_buffer[old_next_slot] = channel;
	}
	else
	{
		/* 0xFF indicates that this is not a normal channel id */
		g_recorder_data_ptr->user_event_buffer.channel_buffer[old_next_slot] = (UserEventChannel)0xFF;
	}
	trcCRITICAL_SECTION_END();
}

/*******************************************************************************
 * trace_register_channel_format
 *
 * Attempts to create a pair of the channel and format string.
 *
 * Note: This is only available if USE_SEPARATE_USER_EVENT_BUFFER is enabled in
 * trcConfig.h
 ******************************************************************************/
UserEventChannel trace_register_channel_format(traceLabel channel, traceLabel formatStr)
{
	uint8_t i;
	UserEventChannel retVal = 0;

	TRACE_ASSERT(formatStr != 0, "vTraceRegisterChannelFormat: formatStr == 0", (UserEventChannel)0);

	trcCRITICAL_SECTION_BEGIN();
	for (i = 1; i <= CHANNEL_FORMAT_PAIRS; i++) /* Size of the channels buffer is CHANNEL_FORMAT_PAIRS + 1. Index 0 is unused. */
	{
		if(g_recorder_data_ptr->user_event_buffer.channels[i].name == 0 && g_recorder_data_ptr->user_event_buffer.channels[i].format == 0)
		{
			/* Found empty slot */
			g_recorder_data_ptr->user_event_buffer.channels[i].name = channel;
			g_recorder_data_ptr->user_event_buffer.channels[i].format = formatStr;
			retVal = (UserEventChannel)i;
			break;
		}

		if (g_recorder_data_ptr->user_event_buffer.channels[i].name == channel && g_recorder_data_ptr->user_event_buffer.channels[i].format == formatStr)
		{
			/* Found a match */
			retVal = (UserEventChannel)i;
			break;
		}
	}
	trcCRITICAL_SECTION_END();
	return retVal;
}

/******************************************************************************
 * trace_channel_printf
 *
 * Slightly faster version of trace_printf() due to no lookups.
 *
 * Note: This is only available if USE_SEPARATE_USER_EVENT_BUFFER is enabled in
 * trcConfig.h
 *
 ******************************************************************************/
void trace_channel_printf(UserEventChannel channelPair, ...)
{
#if (TRACE_SCHEDULING_ONLY == 0)
	va_list vl;

	va_start(vl, channelPair);
	trace_channel_printf_Helper(channelPair, vl);
	va_end(vl);
#endif /* TRACE_SCHEDULING_ONLY */
}

void trace_channel_printf_Helper(UserEventChannel channelPair, va_list vl)
{
	traceLabel channel;
	traceLabel formatStr;

	TRACE_ASSERT(channelPair != 0, "trace_channel_printf: channelPair == 0", );
	TRACE_ASSERT(channelPair <= CHANNEL_FORMAT_PAIRS, "trace_channel_printf: ", );

	channel = g_recorder_data_ptr->user_event_buffer.channels[channelPair].name;
	formatStr = g_recorder_data_ptr->user_event_buffer.channels[channelPair].format;

	prtrace_user_eventHelper1(channelPair, channel, formatStr, vl);
}

/******************************************************************************
 * trace_channel_user_event
 *
 * Slightly faster version of trace_user_event() due to no lookups.
 ******************************************************************************/
void trace_channel_user_event(UserEventChannel channelPair)
{
#if (TRACE_SCHEDULING_ONLY == 0)
	uint32_t data[(3 + MAX_ARG_SIZE) / 4];

	TRACE_ASSERT(channelPair != 0, "trace_channel_printf: channelPair == 0", );
	TRACE_ASSERT(channelPair <= CHANNEL_FORMAT_PAIRS, "trace_channel_printf: ", );

	prtrace_user_eventHelper2(channelPair, data, 1); /* Only need one slot for timestamp */
#endif /* TRACE_SCHEDULING_ONLY */
}
#endif /* USE_SEPARATE_USER_EVENT_BUFFER == 1 */

/******************************************************************************
 * trace_printf
 *
 * Advanced user events (Professional Edition only)
 *
 * Generates User Event with formatted text and data, similar to a "printf".
 * It is very fast compared to a normal "printf" since this function only
 * stores the arguments. The actual formatting is done
 * on the host PC when the trace is displayed in the viewer tool.
 *
 * User Event labels are created using trace_open_label.
 * Example:
 *
 *	 traceLabel adc_uechannel = trace_open_label("ADC User Events");
 *	 ...
 *	 vTracePrint(adc_uechannel,
 *				 "ADC channel %d: %lf volts",
 *				 ch, (double)adc_reading/(double)scale);
 *
 * This can be combined into one line, if desired, but this is slower:
 *
 *	 vTracePrint(trace_open_label("ADC User Events"),
 *				 "ADC channel %d: %lf volts",
 *				 ch, (double)adc_reading/(double)scale);
 *
 * Calling trace_open_label multiple times will not create duplicate entries, but
 * it is of course faster to just do it once, and then keep the handle for later
 * use. If you don�t have any data arguments, only a text label/string, it is
 * better to use trace_user_event - it is faster.
 *
 * Format specifiers supported:
 * %d - 32 bit signed integer
 * %u - 32 bit unsigned integer
 * %f - 32 bit float
 * %s - string (is copied to the recorder symbol table)
 * %hd - 16 bit signed integer
 * %hu - 16 bit unsigned integer
 * %bd - 8 bit signed integer
 * %bu - 8 bit unsigned integer
 * %lf - double-precision float (Note! See below...)
 *
 * Up to 15 data arguments are allowed, with a total size of maximum 32 byte.
 * In case this is exceeded, the user event is changed into an error message.
 *
 * The data is stored in trace buffer, and is packed to allow storing multiple
 * smaller data entries in the same 4-byte record, e.g., four 8-bit values.
 * A string requires two bytes, as the symbol table is limited to 64K. Storing
 * a double (%lf) uses two records, so this is quite costly. Use float (%f)
 * unless the higher precision is really necessary.
 *
 * Note that the double-precision float (%lf) assumes a 64 bit double
 * representation. This does not seem to be the case on e.g. PIC24 and PIC32.
 * Before using a %lf argument on a 16-bit MCU, please verify that
 * "sizeof(double)" actually gives 8 as expected. If not, use %f instead.
 ******************************************************************************/

void trace_printf(traceLabel eventLabel, const char* formatStr, ...)
{
#if (TRACE_SCHEDULING_ONLY == 0)
	va_list vl;

	va_start(vl, formatStr);
	trace_printf_Helper(eventLabel, formatStr, vl);
	va_end(vl);
#endif /* TRACE_SCHEDULING_ONLY */
}

void trace_printf_Helper(traceLabel eventLabel, const char* formatStr, va_list vl)
{
#if (USE_SEPARATE_USER_EVENT_BUFFER == 0)
	uint32_t noOfSlots;
	UserEvent* ue1;
	uint32_t tempDataBuffer[(3 + MAX_ARG_SIZE) / 4];
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	/**************************************************************************
	* The array tempDataBuffer is a local buffer used in a two-phase commit of
	* the event data, since a trace_printf may span over multiple slots in the
	* buffer.
	* This buffer can be made larger, of course, but remember the risk for
	* stack overflow. Note: This should be a LOCAL buffer, must not be made
	* global. That would cause data corruption when two calls to trace_printf
	* from different tasks overlaps (interrupts are only disabled in a small
	* part of this function, otherwise enabled)
	***************************************************************************/

	TRACE_ASSERT(formatStr != NULL, "trace_printf: formatStr == NULL", );

	trcCRITICAL_SECTION_BEGIN();

	if (g_recorder_data_ptr->recorder_actived && (! inExcludedTask || nISRactive) && handle_of_last_logged_task)
	{
		/* First, write the "primary" user event entry in the local buffer, but
		let the event type be "EVENT_BEING_WRITTEN" for now...*/

		ue1 = (UserEvent*)(&tempDataBuffer[0]);

		ue1->type = EVENT_BEING_WRITTEN;	 /* Update this as the last step */

		noOfSlots = prtrace_user_eventFormat(formatStr, vl, (uint8_t*)tempDataBuffer, 4);

		/* Store the format string, with a reference to the channel symbol */
		ue1->payload = trace_open_symbol(formatStr, eventLabel);

		ue1->dts = (uint8_t)trace_get_dts(0xFF);

		 /* trace_get_dts might stop the recorder in some cases... */
		if (g_recorder_data_ptr->recorder_actived)
		{

			/* If the data does not fit in the remaining main buffer, wrap around to
			0 if allowed, otherwise stop the recorder and quit). */
			if (g_recorder_data_ptr->next_index + noOfSlots > g_recorder_data_ptr->max_events)
			{
				#if (TRACE_RECORDER_STORE_MODE == TRACE_STORE_MODE_RING_BUFFER)
				(void)memset(& g_recorder_data_ptr->event_data[g_recorder_data_ptr->next_index * 4],
						0,
						(g_recorder_data_ptr->max_events - g_recorder_data_ptr->next_index)*4);
				g_recorder_data_ptr->next_index = 0;
				g_recorder_data_ptr->is_buffer_full = 1;
				#else

				/* Stop recorder, since the event data will not fit in the
				buffer and not circular buffer in this case... */
				trace_stop();
				#endif
			}

			/* Check if recorder has been stopped (i.e., trace_stop above) */
			if (g_recorder_data_ptr->recorder_actived)
			{
				/* Check that the buffer to be overwritten does not contain any user
				events that would be partially overwritten. If so, they must be "killed"
				by replacing the user event and following data with NULL events (i.e.,
				using a memset to zero).*/
				#if (TRACE_RECORDER_STORE_MODE == TRACE_STORE_MODE_RING_BUFFER)
				check_data_tobe_overwritten_for_multi_entry_events((uint8_t)noOfSlots);
				#endif
				/* Copy the local buffer to the main buffer */
				(void)memcpy(& g_recorder_data_ptr->event_data[g_recorder_data_ptr->next_index * 4],
						tempDataBuffer,
						noOfSlots * 4);

				/* Update the event type, i.e., number of data entries following the
				main USER_EVENT entry (Note: important that this is after the memcpy,
				but within the critical section!)*/
				g_recorder_data_ptr->event_data[g_recorder_data_ptr->next_index * 4] =
				 (uint8_t) ( USER_EVENT + noOfSlots - 1 );

				/* Update the main buffer event index (already checked that it fits in
				the buffer, so no need to check for wrapping)*/

				g_recorder_data_ptr->next_index += noOfSlots;
				g_recorder_data_ptr->nevents += noOfSlots;

				if (g_recorder_data_ptr->next_index >= EVENT_BUFFER_SIZE)
				{
					#if (TRACE_RECORDER_STORE_MODE == TRACE_STORE_MODE_RING_BUFFER)
					/* We have reached the end, but this is a ring buffer. Start from the beginning again. */
					g_recorder_data_ptr->is_buffer_full = 1;
					g_recorder_data_ptr->next_index = 0;
					#else
					/* We have reached the end so we stop. */
					trace_stop();
					#endif
				}
			}

			#if (TRACE_RECORDER_STORE_MODE == TRACE_STORE_MODE_RING_BUFFER)
			/* Make sure the next entry is cleared correctly */
			check_data_tobe_overwritten_for_multi_entry_events(1);
			#endif

		}
	}
	trcCRITICAL_SECTION_END();

#elif (USE_SEPARATE_USER_EVENT_BUFFER == 1)
	/* Use the separate user event buffer */
	traceLabel formatLabel;
	UserEventChannel channel;

	if (g_recorder_data_ptr->recorder_actived && (! inExcludedTask || nISRactive) && handle_of_last_logged_task)
	{
		formatLabel = trace_open_label(formatStr);

		channel = trace_register_channel_format(eventLabel, formatLabel);

		prtrace_user_eventHelper1(channel, eventLabel, formatLabel, vl);
	}
#endif
}

/******************************************************************************
 * trace_user_event
 *
 * Basic user event (Standard and Professional Edition only)
 *
 * Generates a User Event with a text label. The label is created/looked up
 * in the symbol table using trace_open_label.
 ******************************************************************************/
void trace_user_event(traceLabel eventLabel)
{
#if (TRACE_SCHEDULING_ONLY == 0)
#if (USE_SEPARATE_USER_EVENT_BUFFER == 0)
	UserEvent* ue;
	uint8_t dts1;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	TRACE_ASSERT(eventLabel > 0, "trace_user_event: Invalid value for eventLabel", );

	trcCRITICAL_SECTION_BEGIN();
	if (g_recorder_data_ptr->recorder_actived && (! inExcludedTask || nISRactive) && handle_of_last_logged_task)
	{
		dts1 = (uint8_t)trace_get_dts(0xFF);
		ue = (UserEvent*) trace_next_free_event_buffer_slot();
		if (ue != NULL)
		{
			ue->dts = dts1;
			ue->type = USER_EVENT;
			ue->payload = eventLabel;
			trace_update_counters();
		}
	}
	trcCRITICAL_SECTION_END();

#elif (USE_SEPARATE_USER_EVENT_BUFFER == 1)
	UserEventChannel channel;
	uint32_t noOfSlots = 1;
	uint32_t tempDataBuffer[(3 + MAX_ARG_SIZE) / 4];
	if (g_recorder_data_ptr->recorder_actived && (! inExcludedTask || nISRactive) && handle_of_last_logged_task)
	{
		channel = trace_register_channel_format(0, eventLabel);

		if (channel == 0)
		{
			/* We are dealing with an unknown channel format pair */
			noOfSlots++; /* Also need room for channel and format */
			((uint16_t*)tempDataBuffer)[2] = 0;
			((uint16_t*)tempDataBuffer)[3] = eventLabel;
		}

		prtrace_user_eventHelper2(channel, tempDataBuffer, noOfSlots);
	}
#endif
#endif /* TRACE_SCHEDULING_ONLY */
}

/*******************************************************************************
 * trace_open_label
 *
 * Creates user event labels for user event channels or for individual events.
 * User events can be used to log application events and data for display in
 * the visualization tool. A user event is identified by a label, i.e., a string,
 * which is stored in the recorder's symbol table.
 * When logging a user event, a numeric handle (reference) to this string is
 * used to identify the event. This is obtained by calling
 *
 *	 trace_open_label()
 *
 * which adds the string to the symbol table (if not already present)
 * and returns the corresponding handle.
 *
 * This can be used in two ways:
 *
 * 1. The handle is looked up every time, when storing the user event.
 *
 * Example:
 *	 trace_user_event(trace_open_label("MyUserEvent"));
 *
 * 2. The label is registered just once, with the handle stored in an
 * application variable - much like using a file handle.
 *
 * Example:
 *	 myEventHandle = trace_open_label("MyUserEvent");
 *	 ...
 *	 trace_user_event(myEventHandle);
 *
 * The second option is faster since no lookup is required on each event, and
 * therefore recommended for user events that are frequently
 * executed and/or located in time-critical code. The lookup operation is
 * however fairly fast due to the design of the symbol table.
 ******************************************************************************/
traceLabel trace_open_label(const char* label)
{
	TRACE_ASSERT(label != NULL, "trace_open_label: label == NULL", (traceLabel)0);

	return trace_open_symbol(label, 0);
}

#endif

#endif
