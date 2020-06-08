#ifndef __TRCKERNEL_H__
#define __TRCKERNEL_H__

#include "trcKernelPort.h"

#if (USE_TRACEALYZER_RECORDER == 1)

/* Internal functions */

#if !defined INCLUDE_READY_EVENTS || INCLUDE_READY_EVENTS == 1
	void trace_set_ready_events_enabled(int status);
	void trace_store_task_ready(object_handle_t handle);
#else
	void trace_set_ready_events_enabled(int status)
#endif

void trace_store_low_power(uint32_t flag);

void trace_store_taskswitch(object_handle_t task_handle);

void trace_store_kernel_call(uint32_t eventcode, traceObjectClass objectClass, uint32_t byteParam);

void trace_store_kernel_call_with_numeric_param(uint32_t evtcode,
												uint32_t param);

void trace_store_kernel_call_with_param(uint32_t evtcode, traceObjectClass objectClass,
									uint32_t objectNumber, uint32_t param);

void trace_set_task_instance_finished(object_handle_t handle);

void trace_set_priority_property(uint8_t objectclass, object_handle_t id, uint8_t value);

uint8_t trace_get_priority_property(uint8_t objectclass, object_handle_t id);

void trace_set_object_state(uint8_t objectclass, object_handle_t id, uint8_t value);

uint8_t trace_get_object_state(uint8_t objectclass, object_handle_t id);

#if (INCLUDE_OBJECT_DELETE == 1)

void trace_store_object_name_on_close_event(object_handle_t handle,
										traceObjectClass objectclass);

void trace_store_object_properties_on_close_event(object_handle_t handle,
											 traceObjectClass objectclass);
#endif

/* Internal constants for task state */
#define TASK_STATE_INSTANCE_NOT_ACTIVE 0
#define TASK_STATE_INSTANCE_ACTIVE 1

#endif

#endif



