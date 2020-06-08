#include "trcKernel.h"

#if (USE_TRACEALYZER_RECORDER == 1)

#include <stdint.h>

/* Internal variables */
int8_t nISRactive = 0;
object_handle_t handle_of_last_logged_task = 0;
uint8_t inExcludedTask = 0;

#if (INCLUDE_MEMMANG_EVENTS == 1) && (TRACE_SCHEDULING_ONLY == 0)
 /* Current heap usage. Always updated. */
 static uint32_t heap_usage = 0;
#endif

#if (TRACE_SCHEDULING_ONLY == 0)
static uint32_t prvTraceGetParam(uint32_t, uint32_t);
#endif

#if !defined INCLUDE_READY_EVENTS || INCLUDE_READY_EVENTS == 1

static int readyEventsEnabled = 1;

void trace_set_ready_events_enabled(int status)
{
	readyEventsEnabled = status;
}

/*******************************************************************************
 * trace_store_task_ready
 *
 * This function stores a ready state for the task handle sent in as parameter.
 ******************************************************************************/
void trace_store_task_ready(object_handle_t handle)
{
	uint16_t dts3;
	TREvent* tr;
	uint8_t hnd8;

	TRACE_SR_ALLOC_CRITICAL_SECTION();

	if (handle == 0)
	{
		/*  On FreeRTOS v7.3.0, this occurs when creating tasks due to a bad
		placement of the trace macro. In that case, the events are ignored. */
		return;
	}
	
	if (! readyEventsEnabled)
	{
		/* When creating tasks, ready events are also created. If creating 
		a "hidden" (not traced) task, we must therefore disable recording 
		of ready events to avoid an undesired ready event... */
		return;
	}

	TRACE_ASSERT(handle <= NTask, "trace_store_task_ready: Invalid value for handle", );

	if (recorder_busy)
	{
	 /***********************************************************************
	 * This should never occur, as the tick- and kernel call ISR is on lowest
	 * interrupt priority and always are disabled during the critical sections
	 * of the recorder.
	 ***********************************************************************/

	 trace_error("Recorder busy - high priority ISR using syscall? (1)");
	 return;
	}

	trcCRITICAL_SECTION_BEGIN();
	if (g_recorder_data_ptr->recorder_actived) /* Need to repeat this check! */
	{
		if (!TRACE_GET_TASK_FLAG_ISEXCLUDED(handle))
		{
			dts3 = (uint16_t)trace_get_dts(0xFFFF);
			hnd8 = trace_get_8bit_handle(handle);
			tr = (TREvent*)trace_next_free_event_buffer_slot();
			if (tr != NULL)
			{
				tr->type = DIV_TASK_READY;
				tr->dts = dts3;
				tr->objHandle = hnd8;
				trace_update_counters();
			}
		}
	}
	trcCRITICAL_SECTION_END();
}
#endif

/*******************************************************************************
 * trace_store_low_power
 *
 * This function stores a low power state.
 ******************************************************************************/
void trace_store_low_power(uint32_t flag)
{
	uint16_t dts;
	LPEvent* lp;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	TRACE_ASSERT(flag <= 1, "trace_store_low_power: Invalid flag value", );

	if (recorder_busy)
	{
		/***********************************************************************
		* This should never occur, as the tick- and kernel call ISR is on lowest
		* interrupt priority and always are disabled during the critical sections
		* of the recorder.
		***********************************************************************/

		trace_error("Recorder busy - high priority ISR using syscall? (1)");
		return;
	}

	trcCRITICAL_SECTION_BEGIN();
	if (g_recorder_data_ptr->recorder_actived)
	{
		dts = (uint16_t)trace_get_dts(0xFFFF);
		lp = (LPEvent*)trace_next_free_event_buffer_slot();
		if (lp != NULL)
		{
			lp->type = LOW_POWER_BEGIN + ( uint8_t ) flag; /* BEGIN or END depending on flag */
			lp->dts = dts;
			trace_update_counters();
		}
	}
	trcCRITICAL_SECTION_END();
}

/*******************************************************************************
 * trace_store_mem_mang_event
 *
 * This function stores malloc and free events. Each call requires two records,
 * for size and address respectively. The event code parameter (ecode) is applied
 * to the first record (size) and the following address record gets event
 * code "ecode + 1", so make sure this is respected in the event code table.
 * Note: On "free" calls, the signed_size parameter should be negative.
 ******************************************************************************/
#if (INCLUDE_MEMMANG_EVENTS == 1)
void trace_store_mem_mang_event(uint32_t ecode, uint32_t address, int32_t signed_size)
{	
#if (TRACE_SCHEDULING_ONLY == 0)
	uint8_t dts1;
	MemEventSize * ms;
	MemEventAddr * ma;
	uint16_t size_low;
	uint16_t addr_low;
	uint8_t addr_high;
	uint32_t size;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	if (g_recorder_data_ptr == NULL) // This happens in trace_init_data, if using dynamic allocation...
		return;
	
	if (signed_size < 0)
		size = (uint32_t)(- signed_size);
	else
		size = (uint32_t)(signed_size);

	trcCRITICAL_SECTION_BEGIN();
	
	heap_usage += signed_size;
	
	if (g_recorder_data_ptr->recorder_actived)
	{
		/* If it is an ISR or NOT an excluded task, this kernel call will be stored in the trace */
		if (nISRactive || !inExcludedTask)
		{
			dts1 = (uint8_t)trace_get_dts(0xFF);
			size_low = (uint16_t)prvTraceGetParam(0xFFFF, size);
			ms = (MemEventSize *)trace_next_free_event_buffer_slot();

			if (ms != NULL)
			{
				ms->dts = dts1;
				ms->type = NULL_EVENT; /* Updated when all events are written */
				ms->size = size_low;
				trace_update_counters();

				/* Storing a second record with address (signals "failed" if null) */
				#if (HEAP_SIZE_BELOW_16M)
				    /* If the heap address range is within 16 MB, i.e., the upper 8 bits
					of addresses are constant, this optimization avoids storing an extra
					event record by ignoring the upper 8 bit of the address */
					addr_low = address & 0xFFFF;          
					addr_high = (address >> 16) & 0xFF;
				#else
				    /* The whole 32 bit address is stored using a second event record
					for the upper 16 bit */
					addr_low = (uint16_t)prvTraceGetParam(0xFFFF, address);
					addr_high = 0;
				#endif

				ma = (MemEventAddr *) trace_next_free_event_buffer_slot();
				if (ma != NULL)
				{
					ma->addr_low = addr_low;
					ma->addr_high = addr_high;
					ma->type = ( ( uint8_t) ecode ) + 1; /* Note this! */
					ms->type = (uint8_t)ecode;
					trace_update_counters();					
					g_recorder_data_ptr->heap_usage = heap_usage;
				}
			}
		}
	}
	trcCRITICAL_SECTION_END();
#endif /* TRACE_SCHEDULING_ONLY */
}
#endif

/*******************************************************************************
 * trace_store_kernel_call
 *
 * This is the main integration point for storing kernel calls, and
 * is called by the hooks in trcKernelHooks.h (see trcKernelPort.h for event codes).
 ******************************************************************************/
void trace_store_kernel_call(uint32_t ecode, traceObjectClass objectClass, uint32_t objectNumber)
{
#if (TRACE_SCHEDULING_ONLY == 0)
	KernelCall * kse;
	uint16_t dts1;
	uint8_t hnd8;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	TRACE_ASSERT(ecode < 0xFF, "trace_store_kernel_call: ecode >= 0xFF", );
	TRACE_ASSERT(objectClass < TRACE_NCLASSES, "trace_store_kernel_call: objectClass >= TRACE_NCLASSES", );
	TRACE_ASSERT(objectNumber <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectClass], "trace_store_kernel_call: Invalid value for objectNumber", );

	if (recorder_busy)
	{
		/*************************************************************************
		* This may occur if a high-priority ISR is illegally using a system call,
		* or creates a user event.
		* Only ISRs that are disabled by TRACE_ENTER_CRITICAL_SECTION may use system calls
		* or user events (see TRACE_MAX_SYSCALL_INTERRUPT_PRIORITY).
		*************************************************************************/

		trace_error("Recorder busy - high priority ISR using syscall? (2)");
		return;
	}

	if (handle_of_last_logged_task == 0)
	{
		return;
	}

	trcCRITICAL_SECTION_BEGIN();
	if (g_recorder_data_ptr->recorder_actived)
	{
		/* If it is an ISR or NOT an excluded task, this kernel call will be stored in the trace */
		if (nISRactive || !inExcludedTask)
		{
			/* Check if the referenced object or the event code is excluded */
			if (!trace_is_object_excluded(objectClass, (object_handle_t)objectNumber) && !TRACE_GET_EVENT_CODE_FLAG_ISEXCLUDED(ecode))
			{
				dts1 = (uint16_t)trace_get_dts(0xFFFF);
				hnd8 = trace_get_8bit_handle(objectNumber);
				kse = (KernelCall*) trace_next_free_event_buffer_slot();
				if (kse != NULL)
				{
					kse->dts = dts1;
					kse->type = (uint8_t)ecode;
					kse->objHandle = hnd8;
					trace_update_counters();
				}
			}
		}
	}
	trcCRITICAL_SECTION_END();
#endif /* TRACE_SCHEDULING_ONLY */
}

/*******************************************************************************
 * trace_store_kernel_call_with_param
 *
 * Used for storing kernel calls with a handle and a numeric parameter. If the
 * numeric parameter does not fit in one byte, and extra XPS event is inserted
 * before the kernel call event containing the three upper bytes.
 ******************************************************************************/
void trace_store_kernel_call_with_param(uint32_t evtcode,
									traceObjectClass objectClass,
									uint32_t objectNumber,
									uint32_t param)
{
#if (TRACE_SCHEDULING_ONLY == 0)
	KernelCallWithParamAndHandle * kse;
	uint8_t dts2;
	uint8_t hnd8;
	uint8_t p8;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	TRACE_ASSERT(evtcode < 0xFF, "trace_store_kernel_call: evtcode >= 0xFF", );
	TRACE_ASSERT(objectClass < TRACE_NCLASSES, "trace_store_kernel_call_with_param: objectClass >= TRACE_NCLASSES", );
	TRACE_ASSERT(objectNumber <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectClass], "trace_store_kernel_call_with_param: Invalid value for objectNumber", );

	if (recorder_busy)
	{
		/*************************************************************************
		* This may occur if a high-priority ISR is illegally using a system call,
		* or creates a user event.
		* Only ISRs that are disabled by TRACE_ENTER_CRITICAL_SECTION may use system calls
		* or user events (see TRACE_MAX_SYSCALL_INTERRUPT_PRIORITY).
		*************************************************************************/

		trace_error("Recorder busy - high priority ISR using syscall? (3)");
		return;
	}

	trcCRITICAL_SECTION_BEGIN();
	if (g_recorder_data_ptr->recorder_actived && handle_of_last_logged_task && (! inExcludedTask || nISRactive))
	{
		/* Check if the referenced object or the event code is excluded */
		if (!trace_is_object_excluded(objectClass, (object_handle_t)objectNumber) &&
			!TRACE_GET_EVENT_CODE_FLAG_ISEXCLUDED(evtcode))
		{
			dts2 = (uint8_t)trace_get_dts(0xFF);
			p8 = (uint8_t) prvTraceGetParam(0xFF, param);
			hnd8 = trace_get_8bit_handle((object_handle_t)objectNumber);
			kse = (KernelCallWithParamAndHandle*) trace_next_free_event_buffer_slot();
			if (kse != NULL)
			{
				kse->dts = dts2;
				kse->type = (uint8_t)evtcode;
				kse->objHandle = hnd8;
				kse->param = p8;
				trace_update_counters();
			}
		}
	}
	trcCRITICAL_SECTION_END();
#endif /* TRACE_SCHEDULING_ONLY */
}

#if (TRACE_SCHEDULING_ONLY == 0)

/*******************************************************************************
 * prvTraceGetParam
 *
 * Used for storing extra bytes for kernel calls with numeric parameters.
 *
 * May only be called within a critical section!
 ******************************************************************************/
static uint32_t prvTraceGetParam(uint32_t param_max, uint32_t param)
{
	XPSEvent* xps;

	TRACE_ASSERT(param_max == 0xFF || param_max == 0xFFFF,
		"prvTraceGetParam: Invalid value for param_max", param);

	if (param <= param_max)
	{
		return param;
	}
	else
	{
		xps = (XPSEvent*) trace_next_free_event_buffer_slot();
		if (xps != NULL)
		{
			xps->type = DIV_XPS;
			xps->xps_8 = (param & (0xFF00 & ~param_max)) >> 8;
			xps->xps_16 = (param & (0xFFFF0000 & ~param_max)) >> 16;
			trace_update_counters();
		}

		return param & param_max;
	}
}
#endif

/*******************************************************************************
 * trace_store_kernel_call_with_numeric_param
 *
 * Used for storing kernel calls with numeric parameters only. This is
 * only used for traceTASK_DELAY and traceDELAY_UNTIL at the moment.
 ******************************************************************************/
void trace_store_kernel_call_with_numeric_param(uint32_t evtcode, uint32_t param)
{
#if (TRACE_SCHEDULING_ONLY == 0)
	KernelCallWithParam16 * kse;
	uint8_t dts6;
	uint16_t restParam;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	restParam = 0;

	TRACE_ASSERT(evtcode < 0xFF,
		"trace_store_kernel_call_with_numeric_param: Invalid value for evtcode", );

	if (recorder_busy)
	{
		/*************************************************************************
		* This may occur if a high-priority ISR is illegally using a system call,
		* or creates a user event.
		* Only ISRs that are disabled by TRACE_ENTER_CRITICAL_SECTION may use system calls
		* or user events (see TRACE_MAX_SYSCALL_INTERRUPT_PRIORITY).
		*************************************************************************/

		trace_error("Recorder busy - high priority ISR using syscall? (4)");
		return;
	}

	trcCRITICAL_SECTION_BEGIN();
	if (g_recorder_data_ptr->recorder_actived && handle_of_last_logged_task
		&& (! inExcludedTask || nISRactive))
	{
		/* Check if the event code is excluded */
		if (!TRACE_GET_EVENT_CODE_FLAG_ISEXCLUDED(evtcode))
		{
			dts6 = (uint8_t)trace_get_dts(0xFF);
			restParam = (uint16_t)prvTraceGetParam(0xFFFF, param);
			kse = (KernelCallWithParam16*) trace_next_free_event_buffer_slot();
			if (kse != NULL)
			{
				kse->dts = dts6;
				kse->type = (uint8_t)evtcode;
				kse->param = restParam;
				trace_update_counters();
			}
		}
	}
	trcCRITICAL_SECTION_END();
#endif /* TRACE_SCHEDULING_ONLY */
}

/*******************************************************************************
 * trace_store_taskswitch
 * Called by the scheduler from the SWITCHED_OUT hook, and by tarce_start_internal.
 * At this point interrupts are assumed to be disabled!
 ******************************************************************************/
void trace_store_taskswitch(object_handle_t task_handle)
{
	uint16_t dts3;
	TSEvent* ts;
	int8_t skipEvent;
	uint8_t hnd8;
	TRACE_SR_ALLOC_CRITICAL_SECTION();

	skipEvent = 0;

	TRACE_ASSERT(task_handle <= NTask,
		"trace_store_taskswitch: Invalid value for task_handle", );

	/***************************************************************************
	This is used to detect if a high-priority ISRs is illegally using the
	recorder ISR trace functions (trace_store_isr_begin and ...End) while the
	recorder is busy with a task-level event or lower priority ISR event.

	If this is detected, it triggers a call to trace_error with the error
	"Illegal call to trace_store_isr_begin/End". If you get this error, it means
	that the macro trcCRITICAL_SECTION_BEGIN does not disable this ISR, as required.

	Note: Setting recorder_busy is normally handled in our macros
	trcCRITICAL_SECTION_BEGIN and _END, but is needed explicitly in this
	function since critical sections should not be used in the context switch
	event...)
	***************************************************************************/

	/* Skip the event if the task has been excluded, using vTraceExcludeTask */
	if (TRACE_GET_TASK_FLAG_ISEXCLUDED(task_handle))
	{
		skipEvent = 1;
		inExcludedTask = 1;
	}
	else
	{
		inExcludedTask = 0;
	}

	trcCRITICAL_SECTION_BEGIN_ON_CORTEX_M_ONLY();

	/* Skip the event if the same task is scheduled */
	if (task_handle == handle_of_last_logged_task)
	{
		skipEvent = 1;
	}

	if (!g_recorder_data_ptr->recorder_actived)
	{
		skipEvent = 1;
	}

	/* If this event should be logged, log it! */
	if (skipEvent == 0)
	{
		dts3 = (uint16_t)trace_get_dts(0xFFFF);
		handle_of_last_logged_task = task_handle;
		hnd8 = trace_get_8bit_handle(handle_of_last_logged_task);
		ts = (TSEvent*)trace_next_free_event_buffer_slot();

		if (ts != NULL)
		{
			if (trace_get_object_state(TRACE_CLASS_TASK,
				handle_of_last_logged_task) == TASK_STATE_INSTANCE_ACTIVE)
			{
				ts->type = TS_TASK_RESUME;
			}
			else
			{
				ts->type = TS_TASK_BEGIN;
			}

			ts->dts = dts3;
			ts->objHandle = hnd8;

			trace_set_object_state(TRACE_CLASS_TASK,
									handle_of_last_logged_task,
									TASK_STATE_INSTANCE_ACTIVE);

			trace_update_counters();
		}
	}

	trcCRITICAL_SECTION_END_ON_CORTEX_M_ONLY();
}

/*******************************************************************************
 * vTraceStoreNameCloseEvent
 *
 * Updates the symbol table with the name of this object from the dynamic
 * objects table and stores a "close" event, holding the mapping between handle
 * and name (a symbol table handle). The stored name-handle mapping is thus the
 * "old" one, valid up until this point.
 ******************************************************************************/
#if (INCLUDE_OBJECT_DELETE == 1)
void trace_store_object_name_on_close_event(object_handle_t handle,
										traceObjectClass objectclass)
{
	ObjCloseNameEvent * ce;
	const char * name;
	traceLabel idx;

	TRACE_ASSERT(objectclass < TRACE_NCLASSES,
		"trace_store_object_name_on_close_event: objectclass >= TRACE_NCLASSES", );
	TRACE_ASSERT(handle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass],
		"trace_store_object_name_on_close_event: Invalid value for handle", );

	if (g_recorder_data_ptr->recorder_actived)
	{
		uint8_t hnd8 = trace_get_8bit_handle(handle);
		name = TRACE_PROPERTY_NAME_GET(objectclass, handle);
		idx = trace_open_symbol(name, 0);

		// Interrupt disable not necessary, already done in trcHooks.h macro
		ce = (ObjCloseNameEvent*) trace_next_free_event_buffer_slot();
		if (ce != NULL)
		{
			ce->type = EVENTGROUP_OBJCLOSE_NAME + objectclass;
			ce->objHandle = hnd8;
			ce->symbolIndex = idx;
			trace_update_counters();
		}
	}
}

void trace_store_object_properties_on_close_event(object_handle_t handle,
											 traceObjectClass objectclass)
{
	ObjClosePropEvent * pe;

	TRACE_ASSERT(objectclass < TRACE_NCLASSES,
		"trace_store_object_properties_on_close_event: objectclass >= TRACE_NCLASSES", );
	TRACE_ASSERT(handle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass],
		"trace_store_object_properties_on_close_event: Invalid value for handle", );

	if (g_recorder_data_ptr->recorder_actived)
	{
		// Interrupt disable not necessary, already done in trcHooks.h macro
		pe = (ObjClosePropEvent*) trace_next_free_event_buffer_slot();
		if (pe != NULL)
		{
			if (objectclass == TRACE_CLASS_TASK)
			{
				pe->arg1 = TRACE_PROPERTY_ACTOR_PRIORITY(objectclass, handle);
			}
			else
			{
				pe->arg1 = TRACE_PROPERTY_OBJECT_STATE(objectclass, handle);
			}
			pe->type = EVENTGROUP_OBJCLOSE_PROP + objectclass;
			trace_update_counters();
		}
	}
}
#endif

void trace_set_priority_property(uint8_t objectclass, object_handle_t id, uint8_t value)
{
	TRACE_ASSERT(objectclass < TRACE_NCLASSES,
		"trace_set_priority_property: objectclass >= TRACE_NCLASSES", );
	TRACE_ASSERT(id <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass],
		"trace_set_priority_property: Invalid value for id", );

	TRACE_PROPERTY_ACTOR_PRIORITY(objectclass, id) = value;
}

uint8_t trace_get_priority_property(uint8_t objectclass, object_handle_t id)
{
	TRACE_ASSERT(objectclass < TRACE_NCLASSES,
		"trace_get_priority_property: objectclass >= TRACE_NCLASSES", 0);
	TRACE_ASSERT(id <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass],
		"trace_get_priority_property: Invalid value for id", 0);

	return TRACE_PROPERTY_ACTOR_PRIORITY(objectclass, id);
}

void trace_set_object_state(uint8_t objectclass, object_handle_t id, uint8_t value)
{
	TRACE_ASSERT(objectclass < TRACE_NCLASSES,
		"trace_set_object_state: objectclass >= TRACE_NCLASSES", );
	TRACE_ASSERT(id <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass],
		"trace_set_object_state: Invalid value for id", );

	TRACE_PROPERTY_OBJECT_STATE(objectclass, id) = value;
}

uint8_t trace_get_object_state(uint8_t objectclass, object_handle_t id)
{
	TRACE_ASSERT(objectclass < TRACE_NCLASSES,
		"trace_get_object_state: objectclass >= TRACE_NCLASSES", 0);
	TRACE_ASSERT(id <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass],
		"trace_get_object_state: Invalid value for id", 0);

	return TRACE_PROPERTY_OBJECT_STATE(objectclass, id);
}

void trace_set_task_instance_finished(object_handle_t handle)
{
	TRACE_ASSERT(handle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[TRACE_CLASS_TASK],
		"trace_set_task_instance_finished: Invalid value for handle", );

#if (USE_IMPLICIT_IFE_RULES == 1)
	TRACE_PROPERTY_OBJECT_STATE(TRACE_CLASS_TASK, handle) = 0;
#endif
}

#endif
