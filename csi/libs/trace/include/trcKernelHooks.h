#ifndef __TRCKERNELHOOKS_H__
#define __TRCKERNELHOOKS_H__

#if (USE_TRACEALYZER_RECORDER == 1)

#undef INCLUDE_xTaskGetSchedulerState
#define INCLUDE_xTaskGetSchedulerState 1

#undef INCLUDE_xTaskGetCurrentTaskHandle
#define INCLUDE_xTaskGetCurrentTaskHandle 1

#ifndef INCLUDE_OBJECT_DELETE
#define INCLUDE_OBJECT_DELETE 0
#endif

#ifndef INCLUDE_READY_EVENTS
#define INCLUDE_READY_EVENTS 1
#endif

#ifndef INCLUDE_NEW_TIME_EVENTS
#define INCLUDE_NEW_TIME_EVENTS 0
#endif

#if (INCLUDE_OBJECT_DELETE == 1)
/* This macro will remove the task and store it in the event buffer */
#undef trcKERNEL_HOOKS_TASK_DELETE
#define trcKERNEL_HOOKS_TASK_DELETE(SERVICE, pxTCB) \
	trace_store_kernel_call(TRACE_GET_TASK_EVENT_CODE(SERVICE, SUCCESS, CLASS, pxTCB), TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB)); \
	trace_store_object_name_on_close_event(TRACE_GET_TASK_NUMBER(pxTCB), TRACE_CLASS_TASK); \
	trace_store_object_properties_on_close_event(TRACE_GET_TASK_NUMBER(pxTCB), TRACE_CLASS_TASK); \
	trace_set_priority_property(TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB), TRACE_GET_TASK_PRIORITY(pxTCB)); \
	trace_set_object_state(TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB), TASK_STATE_INSTANCE_NOT_ACTIVE); \
	trace_free_object_handle(TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB));
#else
#undef trcKERNEL_HOOKS_TASK_DELETE
#define trcKERNEL_HOOKS_TASK_DELETE(SERVICE, pxTCB)
#endif

#if (INCLUDE_OBJECT_DELETE == 1)
/* This macro will remove the object and store it in the event buffer */
#undef trcKERNEL_HOOKS_OBJECT_DELETE
#define trcKERNEL_HOOKS_OBJECT_DELETE(SERVICE, CLASS, pxObject) \
	trace_store_kernel_call(TRACE_GET_OBJECT_EVENT_CODE(SERVICE, SUCCESS, CLASS, pxObject), TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject), TRACE_GET_OBJECT_NUMBER(CLASS, pxObject)); \
	trace_store_object_name_on_close_event(TRACE_GET_OBJECT_NUMBER(CLASS, pxObject), TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject)); \
	trace_store_object_properties_on_close_event(TRACE_GET_OBJECT_NUMBER(CLASS, pxObject), TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject)); \
	trace_free_object_handle(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject), TRACE_GET_OBJECT_NUMBER(CLASS, pxObject));
#else
#undef trcKERNEL_HOOKS_OBJECT_DELETE
#define trcKERNEL_HOOKS_OBJECT_DELETE(SERVICE, CLASS, pxObject)
#endif

/* This macro will create a task in the object table */
#undef trcKERNEL_HOOKS_TASK_CREATE
#define trcKERNEL_HOOKS_TASK_CREATE(SERVICE, CLASS, pxTCB) \
	TRACE_SET_TASK_NUMBER(pxTCB) \
	trace_set_object_name(TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB), TRACE_GET_TASK_NAME(pxTCB)); \
	trace_set_priority_property(TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB), TRACE_GET_TASK_PRIORITY(pxTCB)); \
	trace_store_kernel_call(TRACE_GET_TASK_EVENT_CODE(SERVICE, SUCCESS, CLASS, pxTCB), TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB));

/* This macro will create a failed create call to create a task */
#undef trcKERNEL_HOOKS_TASK_CREATE_FAILED
#define trcKERNEL_HOOKS_TASK_CREATE_FAILED(SERVICE, CLASS) \
	trace_store_kernel_call(TRACE_GET_TASK_EVENT_CODE(SERVICE, FAILED, CLASS, 0), TRACE_CLASS_TASK, 0);

/* This macro will setup a task in the object table */
#undef trcKERNEL_HOOKS_OBJECT_CREATE
#define trcKERNEL_HOOKS_OBJECT_CREATE(SERVICE, CLASS, pxObject)\
	TRACE_SET_OBJECT_NUMBER(CLASS, pxObject);\
	trace_store_kernel_call(TRACE_GET_OBJECT_EVENT_CODE(SERVICE, SUCCESS, CLASS, pxObject), TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject), TRACE_GET_OBJECT_NUMBER(CLASS, pxObject)); \
	trace_set_object_state(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject), TRACE_GET_OBJECT_NUMBER(CLASS, pxObject), 0);

/* This macro will create a failed create call to create an object */
#undef trcKERNEL_HOOKS_OBJECT_CREATE_FAILED
#define trcKERNEL_HOOKS_OBJECT_CREATE_FAILED(SERVICE, CLASS, kernelClass) \
	trace_store_kernel_call(TRACE_GET_CLASS_EVENT_CODE(SERVICE, FAILED, CLASS, kernelClass), TRACE_GET_CLASS_TRACE_CLASS(CLASS, kernelClass), 0);

/* This macro will create a call to a kernel service with a certain result, with an object as parameter */
#undef trcKERNEL_HOOKS_KERNEL_SERVICE
#define trcKERNEL_HOOKS_KERNEL_SERVICE(SERVICE, RESULT, CLASS, pxObject) \
	trace_store_kernel_call(TRACE_GET_OBJECT_EVENT_CODE(SERVICE, RESULT, CLASS, pxObject), TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject), TRACE_GET_OBJECT_NUMBER(CLASS, pxObject));

/* This macro will set the state for an object */
#undef trcKERNEL_HOOKS_SET_OBJECT_STATE
#define trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, pxObject, STATE) \
	trace_set_object_state(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject), TRACE_GET_OBJECT_NUMBER(CLASS, pxObject), STATE);

/* This macro will flag a certain task as a finished instance */
#undef trcKERNEL_HOOKS_SET_TASK_INSTANCE_FINISHED
#define trcKERNEL_HOOKS_SET_TASK_INSTANCE_FINISHED() \
	trace_set_task_instance_finished(TRACE_GET_TASK_NUMBER(TRACE_GET_CURRENT_TASK()));

#if INCLUDE_READY_EVENTS == 1
/* This macro will create an event to indicate that a task became Ready */
#undef trcKERNEL_HOOKS_MOVED_TASK_TO_READY_STATE
#define trcKERNEL_HOOKS_MOVED_TASK_TO_READY_STATE(pxTCB) \
	trace_store_task_ready(TRACE_GET_TASK_NUMBER(pxTCB));
#else
#undef trcKERNEL_HOOKS_MOVED_TASK_TO_READY_STATE
#define trcKERNEL_HOOKS_MOVED_TASK_TO_READY_STATE(pxTCB)
#endif

/* This macro will update the internal tick counter and call trace_port_get_time_stamp(0) to update the internal counters */
#undef trcKERNEL_HOOKS_INCREMENT_TICK
#define trcKERNEL_HOOKS_INCREMENT_TICK() \
	{ extern uint32_t uiTraceTickCount; uiTraceTickCount++; trace_port_get_time_stamp(0); }

#if INCLUDE_NEW_TIME_EVENTS == 1
/* This macro will create an event indicating that the OS tick count has increased */
#undef trcKERNEL_HOOKS_NEW_TIME
#define trcKERNEL_HOOKS_NEW_TIME(SERVICE, xValue) \
	trace_store_kernel_call_with_numeric_param(SERVICE, xValue);
#else
#undef trcKERNEL_HOOKS_NEW_TIME
#define trcKERNEL_HOOKS_NEW_TIME(SERVICE, xValue)
#endif

/* This macro will create a task switch event to the currently executing task */
#undef trcKERNEL_HOOKS_TASK_SWITCH
#define trcKERNEL_HOOKS_TASK_SWITCH( pxTCB ) \
	trace_store_taskswitch(TRACE_GET_TASK_NUMBER(pxTCB));

/* This macro will create an event to indicate that the task has been suspended */
#undef trcKERNEL_HOOKS_TASK_SUSPEND
#define trcKERNEL_HOOKS_TASK_SUSPEND(SERVICE, pxTCB) \
	trace_store_kernel_call(SERVICE, TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB)); \
	trace_set_task_instance_finished((uint8_t)TRACE_GET_TASK_NUMBER(pxTCB));

/* This macro will create an event to indicate that a task has called a wait/delay function */
#undef trcKERNEL_HOOKS_TASK_DELAY
#define trcKERNEL_HOOKS_TASK_DELAY(SERVICE, pxTCB, xValue) \
	trace_store_kernel_call_with_numeric_param(SERVICE, xValue); \
	trace_set_task_instance_finished((uint8_t)TRACE_GET_TASK_NUMBER(pxTCB));

/* This macro will create an event to indicate that a task has gotten its priority changed */
#undef trcKERNEL_HOOKS_TASK_PRIORITY_CHANGE
#define trcKERNEL_HOOKS_TASK_PRIORITY_CHANGE(SERVICE, pxTCB, uxNewPriority) \
	trace_store_kernel_call_with_param(SERVICE, TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB), trace_get_priority_property(TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB)));\
	trace_set_priority_property(TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB), (uint8_t)uxNewPriority);

/* This macro will create an event to indicate that the task has been resumed */
#undef trcKERNEL_HOOKS_TASK_RESUME
#define trcKERNEL_HOOKS_TASK_RESUME(SERVICE, pxTCB) \
	trace_store_kernel_call(SERVICE, TRACE_CLASS_TASK, TRACE_GET_TASK_NUMBER(pxTCB));
	
#undef trcKERNEL_HOOKS_TIMER_EVENT
#define trcKERNEL_HOOKS_TIMER_EVENT(SERVICE, pxTimer) \
	trace_store_kernel_call(SERVICE, TRACE_CLASS_TIMER, TRACE_GET_TIMER_NUMBER(pxTimer));

/* This macro will create a timer in the object table and assign the timer a trace handle (timer number).*/
#undef trcKERNEL_HOOKS_TIMER_CREATE
#define trcKERNEL_HOOKS_TIMER_CREATE(SERVICE, pxTimer) \
TRACE_SET_TIMER_NUMBER(pxTimer); \
trace_set_object_name(TRACE_CLASS_TIMER, TRACE_GET_TIMER_NUMBER(pxTimer), TRACE_GET_TIMER_NAME(pxTimer)); \
trace_store_kernel_call(SERVICE, TRACE_CLASS_TIMER, TRACE_GET_TIMER_NUMBER(pxTimer));
#endif

#undef trcKERNEL_HOOKS_TIMER_DELETE
#define trcKERNEL_HOOKS_TIMER_DELETE(SERVICE, pxTimer) \
trace_store_kernel_call(SERVICE, TRACE_CLASS_TIMER, TRACE_GET_TIMER_NUMBER(pxTimer)); \
trace_store_object_name_on_close_event(TRACE_GET_TIMER_NUMBER(pxTimer), TRACE_CLASS_TIMER); \
trace_store_object_properties_on_close_event(TRACE_GET_TIMER_NUMBER(pxTimer), TRACE_CLASS_TIMER); \
trace_free_object_handle(TRACE_CLASS_TIMER, TRACE_GET_TIMER_NUMBER(pxTimer));

#endif /* TRCKERNELHOOKS_H */



