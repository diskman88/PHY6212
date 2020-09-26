#include "trcBase.h"

#if (USE_TRACEALYZER_RECORDER == 1)

#include <stdint.h>

/*******************************************************************************
 * Static data initializations
 ******************************************************************************/

/* Tasks and kernel objects can be explicitly excluded from the trace to reduce
buffer usage. This structure handles the exclude flags for all objects and tasks. 
Note that slot 0 is not used, since not a valid handle. */
uint8_t excluded_objects[(TRACE_KERNEL_OBJECT_COUNT + TRACE_NCLASSES) / 8 + 1] = { 0 };

/* Specific events can also be excluded, i.e., by the event code. This can be
used to exclude kernel calls that don't refer to a kernel object, like a delay.
This structure handle the exclude flags for all event codes */
uint8_t excluded_event_codes[NEventCodes / 8 + 1] = { 0 };

/* A set of stacks that keeps track of available object handles for each class.
The stacks are empty initially, meaning that allocation of new handles will be 
based on a counter (for each object class). Any delete operation will
return the handle to the corresponding stack, for reuse on the next allocate.*/
object_handle_table_t object_handle_table = { { 0 }, { 0 }, { 0 } };

/* Initial HWTC_COUNT value, for detecting if the time-stamping source is 
enabled. If using the OS periodic timer for time-stamping, this might not 
have been configured on the earliest events during the startup. */
uint32_t init_hwtc_count;

/*******************************************************************************
 * RecorderData
 *
 * The main data structure. This is the data read by the Tracealyzer tools, 
 * typically through a debugger RAM dump. The recorder uses the pointer 
 * g_recorder_data_ptr for accessing this, to allow for dynamic allocation.
 *
 * On the NXP LPC17xx you may use the secondary RAM bank (AHB RAM) for this
 * purpose. For instance, the LPC1766 has 32 KB AHB RAM which allows for
 * allocating a buffer size of at least 7500 events without affecting the main
 * RAM. To place RecorderData in this RAM bank using IAR Embedded Workbench 
 * for ARM, use this pragma right before the declaration:
 *
 *	 #pragma location="AHB_RAM_MEMORY"
 *
 * This of course works for other hardware architectures with additional RAM
 * banks as well, just replace "AHB_RAM_MEMORY" with the section name from the 
 * linker .map file, or simply the desired address.
 *
 * For portability reasons, we don't add the pragma directly in trcBase.c, but 
 * in a header file included below. To include this header, you need to enable
 * USE_LINKER_PRAGMA, defined in trcConfig.h.
 *
 * If using GCC, you need to modify the declaration as follows:
 *
 *	 recorder_data_t RecorderData __attribute__ ((section ("name"))) = ...
 * 
 * Remember to replace "name" with the correct section name.
 ******************************************************************************/

static void init_start_markers(void);

#if (TRACE_DATA_ALLOCATION == TRACE_DATA_ALLOCATION_STATIC)
#if (USE_LINKER_PRAGMA == 1)
#include "recorderdata_linker_pragma.h"
#endif

recorder_data_t RecorderData;

#endif

recorder_data_t* g_recorder_data_ptr = NULL;

/* This version of the function dynamically allocates the trace data */
void trace_init_trace_data()
{		
	init_hwtc_count = HWTC_COUNT;
	
#if TRACE_DATA_ALLOCATION == TRACE_DATA_ALLOCATION_STATIC
	g_recorder_data_ptr = &RecorderData;
#elif TRACE_DATA_ALLOCATION == TRACE_DATA_ALLOCATION_DYNAMIC
	g_recorder_data_ptr = (recorder_data_t*)TRACE_MALLOC(sizeof(recorder_data_t));
#elif TRACE_DATA_ALLOCATION == TRACE_DATA_ALLOCATION_CUSTOM
	/* DO NOTHING */
#endif


	TRACE_ASSERT(g_recorder_data_ptr != NULL, "trace_init_trace_data, g_recorder_data_ptr == NULL", );

	if (! g_recorder_data_ptr)
	{
		trace_error("No recorder data structure allocated!");
		return;
	}
		
	(void)memset(g_recorder_data_ptr, 0, sizeof(recorder_data_t));

	g_recorder_data_ptr->startmarker0 = 0x00;
	g_recorder_data_ptr->startmarker1 = 0x01;
	g_recorder_data_ptr->startmarker2 = 0x02;
	g_recorder_data_ptr->startmarker3 = 0x03;
	g_recorder_data_ptr->startmarker4 = 0x70;
	g_recorder_data_ptr->startmarker5 = 0x71;
	g_recorder_data_ptr->startmarker6 = 0x72;
	g_recorder_data_ptr->startmarker7 = 0x73;
	g_recorder_data_ptr->startmarker8 = 0xF0;
	g_recorder_data_ptr->startmarker9 = 0xF1;
	g_recorder_data_ptr->startmarker10 = 0xF2;
	g_recorder_data_ptr->startmarker11 = 0xF3;

	g_recorder_data_ptr->version = TRACE_KERNEL_VERSION;
	g_recorder_data_ptr->minor_version = TRACE_MINOR_VERSION;
	g_recorder_data_ptr->irq_priority_order = IRQ_PRIORITY_ORDER;
	g_recorder_data_ptr->file_size = sizeof(recorder_data_t);
  g_recorder_data_ptr->frequency = HWTC_COUNT_FREQ;

	g_recorder_data_ptr->max_events = EVENT_BUFFER_SIZE;

	g_recorder_data_ptr->debugMarker0 = 0xF0F0F0F0;

	g_recorder_data_ptr->is_16bit_handles = USE_16BIT_OBJECT_HANDLES;

	/* This function is kernel specific */
	trace_init_object_property_table();

	g_recorder_data_ptr->debugMarker1 = 0xF1F1F1F1;
	g_recorder_data_ptr->symbol_table.symtab_size = SYMBOL_TABLE_SIZE;
	g_recorder_data_ptr->symbol_table.next_symbol_index = 1;
#if (INCLUDE_FLOAT_SUPPORT == 1)
	g_recorder_data_ptr->float_encoding = 1.0f; /* otherwise already zero */
#endif
	g_recorder_data_ptr->debugMarker2 = 0xF2F2F2F2;
	(void)strncpy(g_recorder_data_ptr->system_info, "Trace Recorder Demo", 80);
	g_recorder_data_ptr->debugMarker3 = 0xF3F3F3F3;
	g_recorder_data_ptr->endmarker0 = 0x0A;
	g_recorder_data_ptr->endmarker1 = 0x0B;
	g_recorder_data_ptr->endmarker2 = 0x0C;
	g_recorder_data_ptr->endmarker3 = 0x0D;
	g_recorder_data_ptr->endmarker4 = 0x71;
	g_recorder_data_ptr->endmarker5 = 0x72;
	g_recorder_data_ptr->endmarker6 = 0x73;
	g_recorder_data_ptr->endmarker7 = 0x74;
	g_recorder_data_ptr->endmarker8 = 0xF1;
	g_recorder_data_ptr->endmarker9 = 0xF2;
	g_recorder_data_ptr->endmarker10 = 0xF3;
	g_recorder_data_ptr->endmarker11 = 0xF4;

#if USE_SEPARATE_USER_EVENT_BUFFER
	g_recorder_data_ptr->user_event_buffer.bufferID = 1;
	g_recorder_data_ptr->user_event_buffer.version = 0;
	g_recorder_data_ptr->user_event_buffer.nslots = USER_EVENT_BUFFER_SIZE;
	g_recorder_data_ptr->user_event_buffer.nchannels = CHANNEL_FORMAT_PAIRS + 1;
#endif

	/* Kernel specific initialization of the object_handle_table variable */
	trace_init_object_handle_stack();

	/* Fix the start markers of the trace data structure */
	init_start_markers();
	
	#ifdef PORT_SPECIFIC_INIT
	PORT_SPECIFIC_INIT();
	#endif
}

static void init_start_markers()
{
	uint32_t i;
	uint8_t *ptr = (uint8_t*)&(g_recorder_data_ptr->startmarker0);
	if ((*ptr) == 0)
	{
		for (i = 0; i < 12; i++)
		{
			ptr[i] += 1;
		}
	}
	else
	{
		trace_error("Trace start markers already initialized!");
	}
}

volatile int recorder_busy = 0;

/* Gives the last error message of the recorder. NULL if no error message. */
char* traceErrorMessage = NULL;

void* trace_next_free_event_buffer_slot(void)
{
	if (! g_recorder_data_ptr->recorder_actived)
	{
		// If the associated XTS or XPS event prio to the main event has filled the buffer and store mode "stop when full".
		return NULL;
	}

	if (g_recorder_data_ptr->next_index >= EVENT_BUFFER_SIZE)
	{
		trace_error("Attempt to index outside event buffer!");
		return NULL;
	}
	return (void*)(&g_recorder_data_ptr->event_data[g_recorder_data_ptr->next_index*4]);
}

uint16_t get_index_of_object(object_handle_t objecthandle, uint8_t objectclass)
{
	TRACE_ASSERT(objectclass < TRACE_NCLASSES, 
		"get_index_of_object: Invalid value for objectclass", 0);
	TRACE_ASSERT(objecthandle > 0 && objecthandle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass], 
		"get_index_of_object: Invalid value for objecthandle", 0);

	if ((objectclass < TRACE_NCLASSES) && (objecthandle > 0) && 
		(objecthandle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass]))
	{
		return (uint16_t)(g_recorder_data_ptr->object_property_table.start_index_of_class[objectclass] + 
			(g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[objectclass] * (objecthandle-1)));
	}

	trace_error("Object table lookup with invalid object handle or object class!");
	return 0;
}

/*******************************************************************************
 * Object handle system
 * This provides a mechanism to assign each kernel object (tasks, queues, etc)
 * with a 1-byte handle, that is used to identify the object in the trace.
 * This way, only one byte instead of four is necessary to identify the object.
 * This allows for maximum 255 objects, of each object class, active at any
 * moment.
 * Note that zero is reserved as an error code and is not a valid handle.
 *
 * In order to allow for fast dynamic allocation and release of object handles,
 * the handles of each object class (e.g., TASK) are stored in a stack. When a
 * handle is needed, e.g., on task creation, the next free handle is popped from
 * the stack. When an object (e.g., task) is deleted, its handle is pushed back
 * on the stack and can thereby be reused for other objects.
 *
 * Since this allows for reuse of object handles, a specific handle (e.g, "8")
 * may refer to TASK_X at one point, and later mean "TASK_Y". To resolve this,
 * the recorder uses "Close events", which are stored in the main event buffer
 * when objects are deleted and their handles are released. The close event
 * contains the mapping between object handle and object name which was valid up
 * to this point in time. The object name is stored as a symbol table entry.
 ******************************************************************************/

object_handle_t trace_get_object_handle(traceObjectClass objectclass)
{
	object_handle_t handle;
	static int indexOfHandle;

	TRACE_ASSERT(objectclass < TRACE_NCLASSES, 
		"trace_get_object_handle: Invalid value for objectclass", (object_handle_t)0);

	indexOfHandle = object_handle_table.available_index[objectclass];
	if (object_handle_table.object_handles[indexOfHandle] == 0)
	{
		/* Zero is used to indicate a never before used handle, i.e.,
			new slots in the handle stack. The handle slot needs to
			be initialized here (starts at 1). */
		object_handle_table.object_handles[indexOfHandle] =
			(object_handle_t)(1 + indexOfHandle -
			object_handle_table.start_index[objectclass]);
	}

	handle = object_handle_table.object_handles[indexOfHandle];

	if (object_handle_table.available_index[objectclass]
		> object_handle_table.start_index[objectclass + 1])
	{
		/* ERROR */
		trace_error(trace_get_error_not_enough_handles(objectclass));

		handle = 0; /* an invalid/anonymous handle - but the recorder is stopped now... */
	}
	else
	{
		int hndCount;
		object_handle_table.available_index[objectclass]++;

		hndCount = object_handle_table.available_index[objectclass] -
			object_handle_table.start_index[objectclass];

		TRACE_CLEAR_OBJECT_FLAG_ISEXCLUDED(objectclass, handle);
	}

	return handle;
}

void trace_free_object_handle(traceObjectClass objectclass, object_handle_t handle)
{
	int indexOfHandle;

	TRACE_ASSERT(objectclass < TRACE_NCLASSES, 
		"trace_free_object_handle: Invalid value for objectclass", );
	TRACE_ASSERT(handle > 0 && handle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass], 
		"trace_free_object_handle: Invalid value for handle", );

	/* Check that there is room to push the handle on the stack */
	if ((object_handle_table.available_index[objectclass] - 1) <
		object_handle_table.start_index[objectclass])
	{
		/* Error */
		trace_error("Attempt to free more handles than allocated!");
	}
	else
	{
		object_handle_table.available_index[objectclass]--;
		indexOfHandle = object_handle_table.available_index[objectclass];
		object_handle_table.object_handles[indexOfHandle] = handle;
	}

}

/*******************************************************************************
 * Objects Property Table
 *
 * This holds the names and properties of the currently active objects, such as
 * tasks and queues. This is developed to support "dynamic" objects which might
 * be deleted during runtime. Their handles are only valid during their
 * lifetime, i.e., from create to delete, as they might be reused on later
 * create operations. When an object is deleted from the OPT, its data is moved
 * to the trace buffer and/or the symbol table.
 * When an object (task, queue, etc.) is created, it receives a handle, which
 * together with the object class specifies its location in the OPT. Thus,
 * objects of different types may share the same name and/or handle, but still
 * be independent objects.
 ******************************************************************************/

/*******************************************************************************
 * trace_set_object_name
 *
 * Registers the names of queues, semaphores and other kernel objects in the
 * recorder's Object Property Table, at the given handle and object class.
 ******************************************************************************/
void trace_set_object_name(traceObjectClass objectclass,
						 object_handle_t handle,
						 const char* name)
{
	static uint16_t idx;

	TRACE_ASSERT(name != NULL, "trace_set_object_name: name == NULL", );

	if (objectclass >= TRACE_NCLASSES)
	{
		trace_error("Illegal object class in trace_set_object_name");
		return;
	}

	if (handle == 0)
	{
		trace_error("Illegal handle (0) in trace_set_object_name.");
		return;
	}

	if (handle > g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass])
	{
		/* ERROR */
		trace_error(trace_get_error_not_enough_handles(objectclass));
	}
	else
	{
		idx = get_index_of_object(handle, objectclass);

		if (traceErrorMessage == NULL)
		{
			(void)strncpy((char*)&(g_recorder_data_ptr->object_property_table.objbytes[idx]),
				name,
				g_recorder_data_ptr->object_property_table.name_length_per_class);
		}
	}
}

traceLabel trace_open_symbol(const char* name, traceLabel userEventChannel)
{
	uint16_t result;
	uint8_t len;
	uint8_t crc;
	TRACE_SR_ALLOC_CRITICAL_SECTION();
	
	len = 0;
	crc = 0;
	
	TRACE_ASSERT(name != NULL, "trace_open_symbol: name == NULL", (traceLabel)0);

	trace_get_checksum(name, &crc, &len);

	trcCRITICAL_SECTION_BEGIN();
	result = trace_lookup_symbol_table_entry(name, crc, len, userEventChannel);
	if (!result)
	{
		result = trace_create_symbol_table_entry(name, crc, len, userEventChannel);
	}
	trcCRITICAL_SECTION_END();

	return result;
}

/*******************************************************************************
 * Supporting functions
 ******************************************************************************/

/*******************************************************************************
 * trace_error
 *
 * Called by various parts in the recorder. Stops the recorder and stores a
 * pointer to an error message, which is printed by the monitor task.
 * If you are not using the monitor task, you may use trace_get_last_error()
 * from your application to check if the recorder is OK.
 *
 * Note: If a recorder error is registered before trace_start is called, the
 * trace start will be aborted. This can occur if any of the Nxxxx constants
 * (e.g., NTask) in trcConfig.h is too small.
 ******************************************************************************/
void trace_error(const char* msg)
{
	TRACE_ASSERT(msg != NULL, "trace_error: msg == NULL", );
	TRACE_ASSERT(g_recorder_data_ptr != NULL, "trace_error: g_recorder_data_ptr == NULL", );

	/* Stop the recorder. Note: We do not call trace_stop, since that adds a weird
	and unnecessary dependency to trcUser.c */

	g_recorder_data_ptr->recorder_actived = 0;

	if (traceErrorMessage == NULL)
	{
		traceErrorMessage = (char*)msg;
		(void)strncpy(g_recorder_data_ptr->system_info, traceErrorMessage, 80);
		g_recorder_data_ptr->internal_error = 1;	 	 
	}
	
}

/******************************************************************************
 * check_data_tobe_overwritten_for_multi_entry_events
 *
 * This checks if the next event to be overwritten is a multi-entry user event,
 * i.e., a USER_EVENT followed by data entries.
 * Such data entries do not have an event code at byte 0, as other events.
 * All 4 bytes are user data, so the first byte of such data events must
 * not be interpreted as type field. The number of data entries following
 * a USER_EVENT is given in the event code of the USER_EVENT.
 * Therefore, when overwriting a USER_EVENT (when using in ringbuffer mode)
 * any data entries following must be replaced with NULL events (code 0).
 *
 * This is assumed to execute within a critical section...
 *****************************************************************************/

void check_data_tobe_overwritten_for_multi_entry_events(uint8_t nofEntriesToCheck)
{
	/* Generic "int" type is desired - should be 16 bit variable on 16 bit HW */
	unsigned int i = 0;
	unsigned int e = 0;

	TRACE_ASSERT(nofEntriesToCheck != 0, 
		"check_data_tobe_overwritten_for_multi_entry_events: nofEntriesToCheck == 0", );

	while (i < nofEntriesToCheck)
	{
		e = g_recorder_data_ptr->next_index + i;
		if ((g_recorder_data_ptr->event_data[e*4] > USER_EVENT) &&
			(g_recorder_data_ptr->event_data[e*4] < USER_EVENT + 16))
		{
			uint8_t nDataEvents = (uint8_t)(g_recorder_data_ptr->event_data[e*4] - USER_EVENT);
			if ((e + nDataEvents) < g_recorder_data_ptr->max_events)
			{
				(void)memset(& g_recorder_data_ptr->event_data[e*4], 0, 4 + 4 * nDataEvents);
			}
		}
		else if (g_recorder_data_ptr->event_data[e*4] == DIV_XPS)
		{
			if ((e + 1) < g_recorder_data_ptr->max_events)
			{
				/* Clear 8 bytes */
				(void)memset(& g_recorder_data_ptr->event_data[e*4], 0, 4 + 4);
			}
			else
			{
				/* Clear 8 bytes, 4 first and 4 last */
				(void)memset(& g_recorder_data_ptr->event_data[0], 0, 4);
				(void)memset(& g_recorder_data_ptr->event_data[e*4], 0, 4);
			}
		}
		i++;
	}
}

/*******************************************************************************
 * trace_update_counters
 *
 * Updates the index of the event buffer.
 ******************************************************************************/
void trace_update_counters(void)
{	
	if (g_recorder_data_ptr->recorder_actived == 0)
	{
		return;
	}
	
	g_recorder_data_ptr->nevents++;

	g_recorder_data_ptr->next_index++;

	if (g_recorder_data_ptr->next_index >= EVENT_BUFFER_SIZE)
	{
#if (TRACE_RECORDER_STORE_MODE == TRACE_STORE_MODE_RING_BUFFER)
		g_recorder_data_ptr->is_buffer_full = 1;
		g_recorder_data_ptr->next_index = 0;
#else
		trace_stop();
#endif
	}

#if (TRACE_RECORDER_STORE_MODE == TRACE_STORE_MODE_RING_BUFFER)
	check_data_tobe_overwritten_for_multi_entry_events(1);
#endif
}

/******************************************************************************
 * trace_get_dts
 *
 * Returns a differential timestamp (DTS), i.e., the time since
 * last event, and creates an XTS event if the DTS does not fit in the
 * number of bits given. The XTS event holds the MSB bytes of the DTS.
 *
 * The parameter param_maxDTS should be 0xFF for 8-bit dts or 0xFFFF for
 * events with 16-bit dts fields.
 *****************************************************************************/
uint16_t trace_get_dts(uint16_t param_maxDTS)
{
	static uint32_t old_timestamp = 0;
	XTSEvent* xts = 0;
	uint32_t dts = 0;
	uint32_t timestamp = 0;

	TRACE_ASSERT(param_maxDTS == 0xFF || param_maxDTS == 0xFFFF, "trace_get_dts: Invalid value for param_maxDTS", 0);


	if (g_recorder_data_ptr->frequency == 0 && init_hwtc_count != HWTC_COUNT)
	{
		/* If HWTC_PERIOD is mapped to the timer reload register,
		it might not be initialized	before the scheduler has been started. 
		We therefore store the frequency of the timer when the counter
		register has changed from its initial value. 
		(Note that this function is called also by trace_start and
		tarce_start_internal, which might be called before the scheduler
		has been started.) */

#if (SELECTED_PORT == PORT_HWIndependent)
		g_recorder_data_ptr->frequency = TRACE_TICK_RATE_HZ;
#else
		g_recorder_data_ptr->frequency = (HWTC_PERIOD * TRACE_TICK_RATE_HZ) / (uint32_t)HWTC_DIVISOR;
#endif
	}

	/**************************************************************************
	* The below statements read the timestamp from the timer port module.
	* If necessary, whole seconds are extracted using division while the rest
	* comes from the modulo operation.
	**************************************************************************/
	
	trace_port_get_time_stamp(&timestamp);	
	
	/***************************************************************************
	* Since dts is unsigned the result will be correct even if timestamp has
	* wrapped around.
	***************************************************************************/
	dts = timestamp - old_timestamp;
	old_timestamp = timestamp;

	if (g_recorder_data_ptr->frequency > 0)
	{
		/* Check if dts > 1 second */
		if (dts > g_recorder_data_ptr->frequency)
		{
			/* More than 1 second has passed */
			g_recorder_data_ptr->abs_time_last_event_second += dts / g_recorder_data_ptr->frequency;
			/* The part that is not an entire second is added to abs_time_last_event */
			g_recorder_data_ptr->abs_time_last_event += dts % g_recorder_data_ptr->frequency;
		}
		else
		{
			g_recorder_data_ptr->abs_time_last_event += dts;
		}

		/* Check if abs_time_last_event >= 1 second */
		if (g_recorder_data_ptr->abs_time_last_event >= g_recorder_data_ptr->frequency)
		{
			/* g_recorder_data_ptr->abs_time_last_event is more than or equal to 1 second, but always less than 2 seconds */
			g_recorder_data_ptr->abs_time_last_event_second++;
			g_recorder_data_ptr->abs_time_last_event -= g_recorder_data_ptr->frequency;
			/* g_recorder_data_ptr->abs_time_last_event is now less than 1 second */
		}
	}
	else
	{
		/* Special case if the recorder has not yet started (frequency may be uninitialized, i.e., zero) */
		g_recorder_data_ptr->abs_time_last_event = timestamp;
	}

	/* If the dts (time since last event) does not fit in event->dts (only 8 or 16 bits) */
	if (dts > param_maxDTS)
	{
		/* Create an XTS event (eXtended TimeStamp) containing the higher dts bits*/
		xts = (XTSEvent*) trace_next_free_event_buffer_slot();

		if (xts != NULL)
		{
			if (param_maxDTS == 0xFFFF)
			{
				xts->type = XTS16;
				xts->xts_16 = (uint16_t)((dts / 0x10000) & 0xFFFF);
				xts->xts_8 = 0;
			}
			else if (param_maxDTS == 0xFF)
			{
				xts->type = XTS8;
				xts->xts_16 = (uint16_t)((dts / 0x100) & 0xFFFF);
				xts->xts_8 = (uint8_t)((dts / 0x1000000) & 0xFF);
			}
			else
			{
				trace_error("Bad param_maxDTS in trace_get_dts");
			}
			trace_update_counters();
		}
	}

	return (uint16_t)dts & param_maxDTS;
}

/*******************************************************************************
 * trace_lookup_symbol_table_entry
 *
 * Find an entry in the symbol table, return 0 if not present.
 *
 * The strings are stored in a byte pool, with four bytes of "meta-data" for
 * every string.
 * byte 0-1: index of next entry with same checksum (for fast lookup).
 * byte 2-3: reference to a symbol table entry, a label for trace_printf
 * format strings only (the handle of the destination channel).
 * byte 4..(4 + length): the string (object name or user event label), with
 * zero-termination
 ******************************************************************************/
traceLabel trace_lookup_symbol_table_entry(const char* name,
										 uint8_t crc6,
										 uint8_t len,
										 traceLabel chn)
{
	uint16_t i = g_recorder_data_ptr->symbol_table.latest_entry_of_checksum[ crc6 ];

	TRACE_ASSERT(name != NULL, "trace_lookup_symbol_table_entry: name == NULL", (traceLabel)0);
	TRACE_ASSERT(len != 0, "trace_lookup_symbol_table_entry: len == 0", (traceLabel)0);

	while (i != 0)
	{
		if (g_recorder_data_ptr->symbol_table.symbytes[i + 2] == (chn & 0x00FF))
		{
			if (g_recorder_data_ptr->symbol_table.symbytes[i + 3] == (chn / 0x100))
			{
				if (g_recorder_data_ptr->symbol_table.symbytes[i + 4 + len] == '\0')
				{
					if (strncmp((char*)(& g_recorder_data_ptr->symbol_table.symbytes[i + 4]), name, len) == 0)
					{
						break; /* found */
					}
				}
			}
		}
		i = (uint16_t)(g_recorder_data_ptr->symbol_table.symbytes[i] + (g_recorder_data_ptr->symbol_table.symbytes[i + 1] * 0x100));
	}
	return i;
}

/*******************************************************************************
 * trace_create_symbol_table_entry
 *
 * Creates an entry in the symbol table, independent if it exists already.
 *
 * The strings are stored in a byte pool, with four bytes of "meta-data" for
 * every string.
 * byte 0-1: index of next entry with same checksum (for fast lookup).
 * byte 2-3: reference to a symbol table entry, a label for trace_printf
 * format strings only (the handle of the destination channel).
 * byte 4..(4 + length): the string (object name or user event label), with
 * zero-termination
 ******************************************************************************/
uint16_t trace_create_symbol_table_entry(const char* name,
										uint8_t crc6,
										uint8_t len,
										traceLabel channel)
{
	uint16_t ret = 0;

	TRACE_ASSERT(name != NULL, "trace_create_symbol_table_entry: name == NULL", 0);
	TRACE_ASSERT(len != 0, "trace_create_symbol_table_entry: len == 0", 0);

	if (g_recorder_data_ptr->symbol_table.next_symbol_index + len + 4 >= SYMBOL_TABLE_SIZE)
	{
		trace_error("Symbol table full. Increase SYMBOL_TABLE_SIZE in trcConfig.h");
		ret = 0;
	}
	else
	{

		g_recorder_data_ptr->symbol_table.symbytes
			[ g_recorder_data_ptr->symbol_table.next_symbol_index] =
			(uint8_t)(g_recorder_data_ptr->symbol_table.latest_entry_of_checksum[ crc6 ] & 0x00FF);

		g_recorder_data_ptr->symbol_table.symbytes
			[ g_recorder_data_ptr->symbol_table.next_symbol_index + 1] =
			(uint8_t)(g_recorder_data_ptr->symbol_table.latest_entry_of_checksum[ crc6 ] / 0x100);

		g_recorder_data_ptr->symbol_table.symbytes
			[ g_recorder_data_ptr->symbol_table.next_symbol_index + 2] =
			(uint8_t)(channel & 0x00FF);

		g_recorder_data_ptr->symbol_table.symbytes
			[ g_recorder_data_ptr->symbol_table.next_symbol_index + 3] =
			(uint8_t)(channel / 0x100);

		/* set name (bytes 4...4+len-1) */
		(void)strncpy((char*)&(g_recorder_data_ptr->symbol_table.symbytes
			[ g_recorder_data_ptr->symbol_table.next_symbol_index + 4]), name, len);

		/* Set zero termination (at offset 4+len) */
		g_recorder_data_ptr->symbol_table.symbytes
			[g_recorder_data_ptr->symbol_table.next_symbol_index + 4 + len] = '\0';

		/* store index of entry (for return value, and as head of LL[crc6]) */
		g_recorder_data_ptr->symbol_table.latest_entry_of_checksum
			[ crc6 ] = (uint16_t)g_recorder_data_ptr->symbol_table.next_symbol_index;

		g_recorder_data_ptr->symbol_table.next_symbol_index += (len + 5);

		ret = (uint16_t)(g_recorder_data_ptr->symbol_table.next_symbol_index -
			(len + 5));
	}

	return ret;
}


/*******************************************************************************
 * trace_get_checksum
 *
 * Calculates a simple 6-bit checksum from a string, used to index the string
 * for fast symbol table lookup.
 ******************************************************************************/
void trace_get_checksum(const char *pname, uint8_t* pcrc, uint8_t* plength)
{
	unsigned char c;
	int length = 1;		/* Should be 1 to account for '\0' */
	int crc = 0;

	TRACE_ASSERT(pname != NULL, "trace_get_checksum: pname == NULL", );
	TRACE_ASSERT(pcrc != NULL, "trace_get_checksum: pcrc == NULL", );
	TRACE_ASSERT(plength != NULL, "trace_get_checksum: plength == NULL", );

	if (pname != (const char *) 0)
	{
		for (; (c = *pname++) != '\0';)
		{
			crc += c;
			length++;
		}
	}
	*pcrc = (uint8_t)(crc & 0x3F);
	*plength = (uint8_t)length;
}

#if (USE_16BIT_OBJECT_HANDLES == 1)

void prvTraceStoreXID(object_handle_t handle); 

/******************************************************************************
 * prvTraceStoreXID
 *
 * Stores an XID (eXtended IDentifier) event.
 * This is used if an object/task handle is larger than 255.
 * The parameter "handle" is the full (16 bit) handle, assumed to be 256 or 
 * larger. Handles below 256 should not use this function.
 *
 * NOTE: this function MUST be called from within a critical section.
 *****************************************************************************/

void prvTraceStoreXID(object_handle_t handle)
{
	XPSEvent* xid;

	TRACE_ASSERT(handle >= 256, "prvTraceStoreXID: Handle < 256", );

	xid = (XPSEvent*)trace_next_free_event_buffer_slot();

	if (xid != NULL)
	{
		xid->type = XID;

		/* This function is (only) used when object_handle_t is 16 bit... */
		xid->xps_16 = handle; 

		trace_update_counters();
	}
}

unsigned char trace_get_8bit_handle(object_handle_t handle)
{
	if (handle > 255)
	{		
		prvTraceStoreXID(handle);
		/* The full handle (16 bit) is stored in the XID event. 
		This code (255) is used instead of zero (which is an error code).*/
		return 255; 
	}
	return (unsigned char)(handle & 0xFF);
}

#endif

#endif
