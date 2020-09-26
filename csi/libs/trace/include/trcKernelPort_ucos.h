#ifndef __TRCKERNELPORTFREERTOS_H__
#define __TRCKERNELPORTFREERTOS_H__

//#include "FreeRTOS.h"	/* Defines configUSE_TRACE_FACILITY */
#include "os.h"
#include "trcHardwarePort.h"
#include "csi_kernel.h"

extern int uiInEventGroupSetBitsFromISR;

#ifdef TRACE_CFG_EN
  #define USE_TRACEALYZER_RECORDER   1
#else
  #define USE_TRACEALYZER_RECORDER   0
#endif

#if (USE_TRACEALYZER_RECORDER == 1)

/* Defines that must be set for the recorder to work properly */
#define TRACE_KERNEL_VERSION 0x1AA1
#define TRACE_CPU_CLOCK_HZ           AHB_DEFAULT_FREQ                             /* Peripheral clock speed in Hertz. */
#define TRACE_PERIPHERAL_CLOCK_HZ    APB_DEFAULT_FREQ                             /* Peripheral clock speed in Hertz. */
#define TRACE_TICK_RATE_HZ           OS_CFG_TICK_RATE_HZ

#if (SELECTED_PORT == PORT_CSKY_ABIV1)
	#define TRACE_SR_ALLOC_CRITICAL_SECTION() CPU_SR_ALLOC ()
	#define TRACE_ENTER_CRITICAL_SECTION()    CPU_CRITICAL_ENTER ();
	#define TRACE_EXIT_CRITICAL_SECTION()     CPU_CRITICAL_EXIT ();
#elif (SELECTED_PORT == PORT_CSKY_ABIV2)
	#define TRACE_SR_ALLOC_CRITICAL_SECTION() CPU_SR_ALLOC ()
	#define TRACE_ENTER_CRITICAL_SECTION()    CPU_CRITICAL_ENTER ();
	#define TRACE_EXIT_CRITICAL_SECTION()     CPU_CRITICAL_EXIT ();
#endif

#ifndef TRACE_ENTER_CRITICAL_SECTION
	#error "This port has no valid definition for critical sections! See http://percepio.com/2014/10/27/how-to-define-critical-sections-for-the-recorder/"
#endif


#define trcCRITICAL_SECTION_BEGIN_ON_CORTEX_M_ONLY() recorder_busy++;
#define trcCRITICAL_SECTION_END_ON_CORTEX_M_ONLY() recorder_busy--;

/*************************************************************************/
/* KERNEL SPECIFIC OBJECT CONFIGURATION									 */
/*************************************************************************/
#define TRACE_NCLASSES 8
#define TRACE_CLASS_QUEUE ((traceObjectClass)0)
#define TRACE_CLASS_SEMAPHORE ((traceObjectClass)1)
#define TRACE_CLASS_MUTEX ((traceObjectClass)2)
#define TRACE_CLASS_TASK ((traceObjectClass)3)
#define TRACE_CLASS_ISR ((traceObjectClass)4)
#define TRACE_CLASS_MEM ((traceObjectClass)5)
#define TRACE_CLASS_TASK_SEM ((traceObjectClass)6)
#define TRACE_CLASS_TASK_Q ((traceObjectClass)7)

#define TRACE_KERNEL_OBJECT_COUNT (NQueue + NSemaphore + NMutex + NTask + NISR + NMem + NTaskQ + NTaskSem)

/* The size of the Object Property Table entries, in bytes, per object */

/* Queue properties (except name):	current number of message in queue */
#define PropertyTableSizeQueue		(OBJECT_NAME_LENGTH + 1)

/* Semaphore properties (except name): state (signaled = 1, cleared = 0) */
#define PropertyTableSizeSemaphore	(OBJECT_NAME_LENGTH + 1)

/* Mutex properties (except name):	owner (task handle, 0 = free) */
#define PropertyTableSizeMutex		(OBJECT_NAME_LENGTH + 1)

/* Task properties (except name):	Byte 0: Current priority
									Byte 1: state (if already active)
									Byte 2: legacy, not used
									Byte 3: legacy, not used */
#define PropertyTableSizeTask		(OBJECT_NAME_LENGTH + 4)

/* ISR properties:					Byte 0: priority
									Byte 1: state (if already active) */
#define PropertyTableSizeISR		(OBJECT_NAME_LENGTH + 2)

/* Mem properties (except name):	state (signaled = 1, cleared = 0) */
#define PropertyTableSizeMem		(OBJECT_NAME_LENGTH + 1)

/* TaskQ properties (except name):	state (signaled = 1, cleared = 0) */
#define PropertyTableSizeTaskQ		(OBJECT_NAME_LENGTH + 1)

/* TaskSem properties (except name):	state (signaled = 1, cleared = 0) */
#define PropertyTableSizeTaskSem		(OBJECT_NAME_LENGTH + 1)


/* The layout of the byte array representing the Object Property Table */
#define StartIndexQueue			0
#define StartIndexSemaphore		StartIndexQueue		+ NQueue		* PropertyTableSizeQueue
#define StartIndexMutex			StartIndexSemaphore + NSemaphore	* PropertyTableSizeSemaphore
#define StartIndexTask			StartIndexMutex		+ NMutex		* PropertyTableSizeMutex
#define StartIndexISR			StartIndexTask		+ NTask			* PropertyTableSizeTask
#define StartIndexMem			StartIndexISR		+ NISR			* PropertyTableSizeISR
#define StartIndexTaskSem			StartIndexMem + NMem			* PropertyTableSizeMem
#define StartIndexTaskQ			StartIndexTaskSem + NTaskSem			* PropertyTableSizeTaskSem

/* Number of bytes used by the object table */
#define TRACE_OBJECT_TABLE_SIZE	StartIndexTaskQ + NTaskQ * PropertyTableSizeTaskQ

/* Includes */
#include "os_type.h" /* Must be first, even before trcTypes.h */
#include "trcConfig.h" /* Must be first, even before trcTypes.h */
//#include "trcHardwarePort.h"
#include "trcTypes.h"
#include "trcKernelHooks.h"
#include "trcBase.h"
#include "trcKernel.h"
#include "trcUser.h"

#if 0
#if (INCLUDE_NEW_TIME_EVENTS == 1 && configUSE_TICKLESS_IDLE != 0)
#error "NewTime events can not be used in combination with tickless idle!"
#endif
#endif

/* Initialization of the object property table */
void trace_init_object_property_table(void);

/* Initialization of the handle mechanism, see e.g, trace_get_object_handle */
void trace_init_object_handle_stack(void);

/* Returns the "Not enough handles" error message for the specified object class */
const char* trace_get_error_not_enough_handles(traceObjectClass objectclass);

/*******************************************************************************
 * The event codes - should match the offline config file.
 *
 * Some sections below are encoded to allow for constructions like:
 *
 * trace_store_kernel_call(EVENTGROUP_CREATE + objectclass, ...
 *
 * The object class ID is given by the three LSB bits, in such cases. Since each
 * object class has a separate object property table, the class ID is needed to
 * know what section in the object table to use for getting an object name from
 * an object handle.
 ******************************************************************************/

#define NULL_EVENT					(0x00) /* Ignored in the analysis*/

/*******************************************************************************
 * EVENTGROUP_DIV
 *
 * Miscellaneous events.
 ******************************************************************************/
#define EVENTGROUP_DIV				(NULL_EVENT + 1)					/*0x01*/
#define DIV_XPS						(EVENTGROUP_DIV + 0)				/*0x01*/
#define DIV_TASK_READY				(EVENTGROUP_DIV + 1)				/*0x02*/
#define DIV_NEW_TIME				(EVENTGROUP_DIV + 2)				/*0x03*/

/*******************************************************************************
 * EVENTGROUP_TS
 *
 * Events for storing task-switches and interrupts. The RESUME events are
 * generated if the task/interrupt is already marked active.
 ******************************************************************************/
#define EVENTGROUP_TS				(EVENTGROUP_DIV + 3)				/*0x04*/
#define TS_ISR_BEGIN				(EVENTGROUP_TS + 0)					/*0x04*/
#define TS_ISR_RESUME				(EVENTGROUP_TS + 1)					/*0x05*/
#define TS_TASK_BEGIN				(EVENTGROUP_TS + 2)					/*0x06*/
#define TS_TASK_RESUME				(EVENTGROUP_TS + 3)					/*0x07*/

/*******************************************************************************
 * EVENTGROUP_OBJCLOSE_NAME
 *
 * About Close Events
 * When an object is evicted from the object property table (object close), two
 * internal events are stored (EVENTGROUP_OBJCLOSE_NAME and
 * EVENTGROUP_OBJCLOSE_PROP), containing the handle-name mapping and object
 * properties valid up to this point.
 ******************************************************************************/
#define EVENTGROUP_OBJCLOSE_NAME	(EVENTGROUP_TS + 4)					/*0x08*/

/*******************************************************************************
 * EVENTGROUP_OBJCLOSE_PROP
 *
 * The internal event carrying properties of deleted objects
 * The handle and object class of the closed object is not stored in this event,
 * but is assumed to be the same as in the preceding CLOSE event. Thus, these
 * two events must be generated from within a critical section.
 * When queues are closed, arg1 is the "state" property (i.e., number of
 * buffered messages/signals).
 * When actors are closed, arg1 is priority, arg2 is handle of the "instance
 * finish" event, and arg3 is event code of the "instance finish" event.
 * In this case, the lower three bits is the object class of the instance finish
 * handle. The lower three bits are not used (always zero) when queues are
 * closed since the queue type is given in the previous OBJCLOSE_NAME event.
 ******************************************************************************/
#define EVENTGROUP_OBJCLOSE_PROP	(EVENTGROUP_OBJCLOSE_NAME + 8)		/*0x10*/

/*******************************************************************************
 * EVENTGROUP_CREATE
 *
 * The events in this group are used to log Kernel object creations.
 * The lower three bits in the event code gives the object class, i.e., type of
 * create operation (task, queue, semaphore, etc).
 ******************************************************************************/
#define EVENTGROUP_CREATE_OBJ_SUCCESS	(EVENTGROUP_OBJCLOSE_PROP + 8)	/*0x18*/

/*******************************************************************************
 * EVENTGROUP_SEND
 *
 * The events in this group are used to log Send/Give events on queues,
 * semaphores and mutexes The lower three bits in the event code gives the
 * object class, i.e., what type of object that is operated on (queue, semaphore
 * or mutex).
 ******************************************************************************/
#define EVENTGROUP_SEND_SUCCESS	(EVENTGROUP_CREATE_OBJ_SUCCESS + 8)		/*0x20*/

/*******************************************************************************
 * EVENTGROUP_RECEIVE
 *
 * The events in this group are used to log Receive/Take events on queues,
 * semaphores and mutexes. The lower three bits in the event code gives the
 * object class, i.e., what type of object that is operated on (queue, semaphore
 * or mutex).
 ******************************************************************************/
#define EVENTGROUP_RECEIVE_SUCCESS	(EVENTGROUP_SEND_SUCCESS + 8)		/*0x28*/

/* Send/Give operations, from ISR */
#define EVENTGROUP_SEND_FROM_TASK_SUCCESS \
									(EVENTGROUP_RECEIVE_SUCCESS + 8)	/*0x30*/

/* Receive/Take operations, from ISR */
#define EVENTGROUP_RECEIVE_FROM_TASK_SUCCESS \
							(EVENTGROUP_SEND_FROM_TASK_SUCCESS + 8)		/*0x38*/

/* "Failed" event type versions of above (timeout, failed allocation, etc) */
#define EVENTGROUP_KSE_FAILED \
							(EVENTGROUP_RECEIVE_FROM_TASK_SUCCESS + 8)	/*0x40*/

/* Failed create calls - memory allocation failed */
#define EVENTGROUP_CREATE_OBJ_FAILED	(EVENTGROUP_KSE_FAILED)			/*0x40*/

/* Failed send/give - timeout! */
#define EVENTGROUP_SEND_FAILED		(EVENTGROUP_CREATE_OBJ_FAILED + 8)	/*0x48*/

/* Failed receive/take - timeout! */
#define EVENTGROUP_RECEIVE_FAILED	 (EVENTGROUP_SEND_FAILED + 8)		/*0x50*/

/* Failed non-blocking send/give - queue full */
#define EVENTGROUP_SEND_FROM_TASK_FAILED (EVENTGROUP_RECEIVE_FAILED + 8) /*0x58*/

/* Failed non-blocking receive/take - queue empty */
#define EVENTGROUP_RECEIVE_FROM_TASK_FAILED \
								 (EVENTGROUP_SEND_FROM_TASK_FAILED + 8)	/*0x60*/

/* Events when blocking on receive/take */
#define EVENTGROUP_RECEIVE_BLOCK \
							(EVENTGROUP_RECEIVE_FROM_TASK_FAILED + 8)	/*0x68*/

/* Events when blocking on send/give */
#define EVENTGROUP_SEND_BLOCK	(EVENTGROUP_RECEIVE_BLOCK + 8)			/*0x70*/

/* Events on queue peek (receive) */
#define EVENTGROUP_PEEK_SUCCESS	(EVENTGROUP_SEND_BLOCK + 8)				/*0x78*/

/* Events on object delete (vTaskDelete or vQueueDelete) */
#define EVENTGROUP_DELETE_OBJ_SUCCESS	(EVENTGROUP_PEEK_SUCCESS + 8)	/*0x80*/

/* Other events - object class is implied: TASK */
#define EVENTGROUP_OTHERS	(EVENTGROUP_DELETE_OBJ_SUCCESS + 8)			/*0x88*/
#define TASK_DELAY_UNTIL	(EVENTGROUP_OTHERS + 0)						/*0x88*/
#define TASK_DELAY			(EVENTGROUP_OTHERS + 1)						/*0x89*/
#define TASK_SUSPEND		(EVENTGROUP_OTHERS + 2)						/*0x8A*/
#define TASK_RESUME			(EVENTGROUP_OTHERS + 3)						/*0x8B*/
#define TASK_RESUME_FROM_ISR	(EVENTGROUP_OTHERS + 4)					/*0x8C*/
#define TASK_PRIORITY_SET		(EVENTGROUP_OTHERS + 5)					/*0x8D*/
#define TASK_PRIORITY_INHERIT	(EVENTGROUP_OTHERS + 6)					/*0x8E*/
#define TASK_PRIORITY_DISINHERIT	(EVENTGROUP_OTHERS + 7)				/*0x8F*/

#define EVENTGROUP_MISC_PLACEHOLDER	(EVENTGROUP_OTHERS + 8)				/*0x90*/
#define PEND_FUNC_CALL		(EVENTGROUP_MISC_PLACEHOLDER+0)				/*0x90*/
#define PEND_FUNC_CALL_FROM_ISR (EVENTGROUP_MISC_PLACEHOLDER+1)			/*0x91*/
#define PEND_FUNC_CALL_FAILED (EVENTGROUP_MISC_PLACEHOLDER+2)			/*0x92*/
#define PEND_FUNC_CALL_FROM_ISR_FAILED (EVENTGROUP_MISC_PLACEHOLDER+3)	/*0x93*/
#define MEM_MALLOC_SIZE (EVENTGROUP_MISC_PLACEHOLDER+4)					/*0x94*/
#define MEM_MALLOC_ADDR (EVENTGROUP_MISC_PLACEHOLDER+5)					/*0x95*/
#define MEM_FREE_SIZE (EVENTGROUP_MISC_PLACEHOLDER+6)					/*0x96*/
#define MEM_FREE_ADDR (EVENTGROUP_MISC_PLACEHOLDER+7)					/*0x97*/

/* User events */
#define EVENTGROUP_USEREVENT (EVENTGROUP_MISC_PLACEHOLDER + 8)			/*0x98*/
#define USER_EVENT (EVENTGROUP_USEREVENT + 0)

/* Allow for 0-15 arguments (the number of args is added to event code) */
#define USER_EVENT_LAST (EVENTGROUP_USEREVENT + 15)						/*0xA7*/

/*******************************************************************************
 * XTS Event - eXtended TimeStamp events
 * The timestamps used in the recorder are "differential timestamps" (DTS), i.e.
 * the time since the last stored event. The DTS fields are either 1 or 2 bytes
 * in the other events, depending on the bytes available in the event struct.
 * If the time since the last event (the DTS) is larger than allowed for by
 * the DTS field of the current event, an XTS event is inserted immediately
 * before the original event. The XTS event contains up to 3 additional bytes
 * of the DTS value - the higher bytes of the true DTS value. The lower 1-2
 * bytes are stored in the normal DTS field.
 * There are two types of XTS events, XTS8 and XTS16. An XTS8 event is stored
 * when there is only room for 1 byte (8 bit) DTS data in the original event,
 * which means a limit of 0xFF (255). The XTS16 is used when the original event
 * has a 16 bit DTS field and thereby can handle values up to 0xFFFF (65535).
 *
 * Using a very high frequency time base can result in many XTS events.
 * Preferably, the time between two OS ticks should fit in 16 bits, i.e.,
 * at most 65535. If your time base has a higher frequency, you can define
 * the TRACE
 ******************************************************************************/

#define EVENTGROUP_SYS (EVENTGROUP_USEREVENT + 16)						/*0xA8*/
#define XTS8 (EVENTGROUP_SYS + 0)										/*0xA8*/
#define XTS16 (EVENTGROUP_SYS + 1)										/*0xA9*/
#define EVENT_BEING_WRITTEN (EVENTGROUP_SYS + 2)						/*0xAA*/
#define RESERVED_DUMMY_CODE (EVENTGROUP_SYS + 3)						/*0xAB*/
#define LOW_POWER_BEGIN (EVENTGROUP_SYS + 4)							/*0xAC*/
#define LOW_POWER_END (EVENTGROUP_SYS + 5)								/*0xAD*/
#define XID (EVENTGROUP_SYS + 6)										/*0xAE*/
#define XTS16L (EVENTGROUP_SYS + 7)										/*0xAF*/

#define EVENTGROUP_TIMER (EVENTGROUP_SYS + 8)							/*0xB0*/
#define TIMER_CREATE (EVENTGROUP_TIMER + 0)								/*0xB0*/
#define TIMER_START (EVENTGROUP_TIMER + 1)								/*0xB1*/
#define TIMER_RST (EVENTGROUP_TIMER + 2)								/*0xB2*/
#define TIMER_STOP (EVENTGROUP_TIMER + 3)								/*0xB3*/
#define TIMER_CHANGE_PERIOD (EVENTGROUP_TIMER + 4)						/*0xB4*/
#define TIMER_DELETE (EVENTGROUP_TIMER + 5)								/*0xB5*/
#define TIMER_START_FROM_ISR (EVENTGROUP_TIMER + 6)						/*0xB6*/
#define TIMER_RESET_FROM_ISR (EVENTGROUP_TIMER + 7)						/*0xB7*/
#define TIMER_STOP_FROM_ISR (EVENTGROUP_TIMER + 8)						/*0xB8*/

#define TIMER_CREATE_FAILED (EVENTGROUP_TIMER + 9)						/*0xB9*/
#define TIMER_START_FAILED (EVENTGROUP_TIMER + 10)						/*0xBA*/
#define TIMER_RESET_FAILED (EVENTGROUP_TIMER + 11)						/*0xBB*/
#define TIMER_STOP_FAILED (EVENTGROUP_TIMER + 12)						/*0xBC*/
#define TIMER_CHANGE_PERIOD_FAILED (EVENTGROUP_TIMER + 13)				/*0xBD*/
#define TIMER_DELETE_FAILED (EVENTGROUP_TIMER + 14)						/*0xBE*/
#define TIMER_START_FROM_ISR_FAILED (EVENTGROUP_TIMER + 15)				/*0xBF*/
#define TIMER_RESET_FROM_ISR_FAILED (EVENTGROUP_TIMER + 16)				/*0xC0*/
#define TIMER_STOP_FROM_ISR_FAILED (EVENTGROUP_TIMER + 17)				/*0xC1*/

#define EVENTGROUP_EG (EVENTGROUP_TIMER + 18)							/*0xC2*/
#define EVENT_GROUP_CREATE (EVENTGROUP_EG + 0)							/*0xC2*/
#define EVENT_GROUP_CREATE_FAILED (EVENTGROUP_EG + 1)					/*0xC3*/
#define EVENT_GROUP_SYNC_BLOCK (EVENTGROUP_EG + 2)						/*0xC4*/
#define EVENT_GROUP_SYNC_END (EVENTGROUP_EG + 3)						/*0xC5*/
#define EVENT_GROUP_WAIT_BITS_BLOCK (EVENTGROUP_EG + 4)					/*0xC6*/
#define EVENT_GROUP_WAIT_BITS_END (EVENTGROUP_EG + 5)					/*0xC7*/
#define EVENT_GROUP_CLEAR_BITS (EVENTGROUP_EG + 6)						/*0xC8*/
#define EVENT_GROUP_CLEAR_BITS_FROM_ISR (EVENTGROUP_EG + 7)				/*0xC9*/
#define EVENT_GROUP_SET_BITS (EVENTGROUP_EG + 8)						/*0xCA*/
#define EVENT_GROUP_DELETE (EVENTGROUP_EG + 9)							/*0xCB*/
#define EVENT_GROUP_SYNC_END_FAILED (EVENTGROUP_EG + 10)				/*0xCC*/
#define EVENT_GROUP_WAIT_BITS_END_FAILED (EVENTGROUP_EG + 11)			/*0xCD*/
#define EVENT_GROUP_SET_BITS_FROM_ISR (EVENTGROUP_EG + 12)				/*0xCE*/
#define EVENT_GROUP_SET_BITS_FROM_ISR_FAILED (EVENTGROUP_EG + 13)		/*0xCF*/

#define TASK_INSTANCE_FINISHED_NEXT_KSE (EVENTGROUP_EG + 14)			/*0xD0*/
#define TASK_INSTANCE_FINISHED_DIRECT (EVENTGROUP_EG + 15)				/*0xD1*/

#define TRACE_TASK_NOTIFY_GROUP (EVENTGROUP_EG + 16)					/*0xD2*/
#define TRACE_TASK_NOTIFY (TRACE_TASK_NOTIFY_GROUP + 0)					/*0xD2*/
#define TRACE_TASK_NOTIFY_TAKE (TRACE_TASK_NOTIFY_GROUP + 1)			/*0xD3*/
#define TRACE_TASK_NOTIFY_TAKE_BLOCK (TRACE_TASK_NOTIFY_GROUP + 2)		/*0xD4*/
#define TRACE_TASK_NOTIFY_TAKE_FAILED (TRACE_TASK_NOTIFY_GROUP + 3)		/*0xD5*/
#define TRACE_TASK_NOTIFY_WAIT (TRACE_TASK_NOTIFY_GROUP + 4)			/*0xD6*/
#define TRACE_TASK_NOTIFY_WAIT_BLOCK (TRACE_TASK_NOTIFY_GROUP + 5)		/*0xD7*/
#define TRACE_TASK_NOTIFY_WAIT_FAILED (TRACE_TASK_NOTIFY_GROUP + 6)		/*0xD8*/
#define TRACE_TASK_NOTIFY_FROM_ISR (TRACE_TASK_NOTIFY_GROUP + 7)		/*0xD9*/
#define TRACE_TASK_NOTIFY_GIVE_FROM_ISR (TRACE_TASK_NOTIFY_GROUP + 8)	/*0xDA*/

/************************************************************************/
/* KERNEL SPECIFIC DATA AND FUNCTIONS NEEDED TO PROVIDE THE				*/
/* FUNCTIONALITY REQUESTED BY THE TRACE RECORDER						*/
/************************************************************************/

/******************************************************************************
 * TraceObjectClassTable
 * Translates a FreeRTOS QueueType into trace objects classes (TRACE_CLASS_).
 * This was added since we want to map both types of Mutex and both types of
 * Semaphores on common classes for all Mutexes and all Semaphores respectively.
 *
 * FreeRTOS Queue types
 * #define queueQUEUE_TYPE_BASE					(0U) => TRACE_CLASS_QUEUE
 * #define queueQUEUE_TYPE_MUTEX				(1U) => TRACE_CLASS_MUTEX
 * #define queueQUEUE_TYPE_COUNTING_SEMAPHORE	(2U) => TRACE_CLASS_SEMAPHORE
 * #define queueQUEUE_TYPE_BINARY_SEMAPHORE		(3U) => TRACE_CLASS_SEMAPHORE
 * #define queueQUEUE_TYPE_RECURSIVE_MUTEX		(4U) => TRACE_CLASS_MUTEX
 ******************************************************************************/

extern traceObjectClass TraceObjectClassTable[5];

/* These functions are implemented in the .c file since certain header files
must not be included in this one */
unsigned char trace_get_object_type(void* handle);
object_handle_t trace_get_task_number(void* handle);
unsigned char trace_is_scheduler_active(void);
unsigned char trace_is_scheduler_suspended(void);
unsigned char trace_is_scheduler_started(void);
void* trace_get_current_task_handle(void);

#if (configUSE_TIMERS == 1)
#undef INCLUDE_xTimerGetTimerDaemonTaskHandle
#define INCLUDE_xTimerGetTimerDaemonTaskHandle 1
#endif

/************************************************************************/
/* KERNEL SPECIFIC MACROS USED BY THE TRACE RECORDER					*/
/************************************************************************/

#define TRACE_MALLOC(size) pvPortMalloc(size)
#define TRACE_IS_SCHEDULER_ACTIVE() trace_is_scheduler_active()
#define TRACE_IS_SCHEDULER_STARTED() trace_is_scheduler_started()
#define TRACE_IS_SCHEDULER_SUSPENDED() trace_is_scheduler_suspended()
#define TRACE_GET_CURRENT_TASK() trace_get_current_task_handle()
#define TRACE_GET_NEXT_TASK() trace_get_next_task_handle()

#define TRACE_GET_TASK_PRIORITY(pxTCB) ((uint8_t)pxTCB->Prio)
#define TRACE_GET_TASK_NAME(pxTCB) ((char*)pxTCB->NamePtr)
#define TRACE_GET_TASK_NUMBER(pxTCB) (trace_get_task_number(pxTCB))
#define TRACE_SET_TASK_NUMBER(pxTCB) pxTCB->TaskID = trace_get_object_handle(TRACE_CLASS_TASK);

#define TRACE_GET_CLASS_TRACE_CLASS(CLASS, kernelClass) TraceObjectClassTable[kernelClass]
#define TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject) TRACE_GET_##CLASS##_TRACE_CLASS()

#define TRACE_GET_TIMER_NUMBER(tmr) ( ( object_handle_t ) ((Timer_t*)tmr)->uxTimerNumber )
#define TRACE_SET_TIMER_NUMBER(tmr) ((Timer_t*)tmr)->uxTimerNumber = trace_get_object_handle(TRACE_CLASS_TIMER);
#define TRACE_GET_TIMER_NAME(pxTimer) pxTimer->pcTimerName
#define TRACE_GET_TIMER_PERIOD(pxTimer) pxTimer->xTimerPeriodInTicks

#define TRACE_GET_EVENTGROUP_NUMBER(eg) ( ( object_handle_t ) uxEventGroupGetNumber(eg) )
#define TRACE_SET_EVENTGROUP_NUMBER(eg) ((EventGroup_t*)eg)->uxEventGroupNumber = trace_get_object_handle(TRACE_CLASS_EVENTGROUP);


#define TRACE_GET_OBJECT_NUMBER(CLASS, pxObject) (TRACE_GET_##CLASS##_NUMBER(pxObject))
#define TRACE_SET_OBJECT_NUMBER(CLASS, pxObject) (TRACE_SET_##CLASS##_NUMBER(pxObject))

#define TRACE_GET_CLASS_EVENT_CODE(SERVICE, RESULT, CLASS, kernelClass) (uint8_t)(EVENTGROUP_##SERVICE##_##RESULT + TRACE_GET_CLASS_TRACE_CLASS(CLASS, kernelClass))
#define TRACE_GET_OBJECT_EVENT_CODE(SERVICE, RESULT, CLASS, pxObject) (uint8_t)(EVENTGROUP_##SERVICE##_##RESULT + TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxObject))
#define TRACE_GET_TASK_EVENT_CODE(SERVICE, RESULT, CLASS, pxTCB) (uint8_t)(EVENTGROUP_##SERVICE##_##RESULT + TRACE_CLASS_TASK)

#define TRACE_GET_TASK_MSG_Q_NUMBER(pmsgq) (((OS_MSG_Q *)pmsgq)->MsgQID)
#define TRACE_SET_TASK_MSG_Q_NUMBER(pmsgq) (((OS_MSG_Q *)pmsgq)->MsgQID = trace_get_object_handle(TRACE_CLASS_TASK_Q))

#define TRACE_GET_TASK_SEM_NUMBER(ptcb) (((OS_TCB *)ptcb)->SemID)
#define TRACE_SET_TASK_SEM_NUMBER(ptcb) (((OS_TCB *)ptcb)->SemID= trace_get_object_handle(TRACE_CLASS_TASK_SEM))

#define TRACE_GET_MUTEX_NUMBER(pmutex) (((OS_MUTEX *)pmutex)->MutexID)
#define TRACE_SET_MUTEX_NUMBER(pmutex) (((OS_MUTEX *)pmutex)->MutexID= trace_get_object_handle(TRACE_CLASS_MUTEX))

#define TRACE_GET_SEM_NUMBER(psem) (((OS_SEM *)psem)->SemID)
#define TRACE_SET_SEM_NUMBER(psem) (((OS_SEM *)psem)->SemID= trace_get_object_handle(TRACE_CLASS_SEMAPHORE))

#define TRACE_GET_Q_NUMBER(pq) (((OS_Q *)pq)->MsgQID)
#define TRACE_SET_Q_NUMBER(pq) (((OS_Q *)pq)->MsgQID = trace_get_object_handle(TRACE_CLASS_QUEUE))

#define TRACE_GET_MEM_NUMBER(pmem) (((OS_MEM *)pmem)->MemID)
#define TRACE_SET_MEM_NUMBER(pmem) (((OS_MEM *)pmem)->MemID= trace_get_object_handle(TRACE_CLASS_MEM))


#define TRACE_GET_TASK_TRACE_CLASS()       TRACE_CLASS_TASK
#define TRACE_GET_Q_TRACE_CLASS()          TRACE_CLASS_QUEUE
#define TRACE_GET_SEM_TRACE_CLASS()        TRACE_CLASS_SEMAPHORE
#define TRACE_GET_MUTEX_TRACE_CLASS()      TRACE_CLASS_MUTEX
#define TRACE_GET_MEM_TRACE_CLASS()        TRACE_CLASS_MEM
#define TRACE_GET_TASK_SEM_TRACE_CLASS()   TRACE_CLASS_TASK_SEM
#define TRACE_GET_TASK_MSG_Q_TRACE_CLASS() TRACE_CLASS_TASK_Q

/************************************************************************/
/* KERNEL SPECIFIC WRAPPERS THAT SHOULD BE CALLED BY THE KERNEL		 */
/************************************************************************/

/* A macro that will update the tick count when returning from tickless idle */
#undef traceINCREASE_TICK_COUNT
/* Note: This can handle time adjustments of max 2^32 ticks, i.e., 35 seconds at 120 MHz. Thus, tick-less idle periods longer than 2^32 ticks will appear "compressed" on the time line.*/
#define traceINCREASE_TICK_COUNT( xCount ) { DWT_CYCLES_ADDED += (xCount * (TRACE_CPU_CLOCK_HZ / TRACE_TICK_RATE_HZ)); }

/* Called for each task that becomes ready */
#undef traceMOVED_TASK_TO_READY_STATE
#define traceMOVED_TASK_TO_READY_STATE( pxTCB ) \
	trcKERNEL_HOOKS_MOVED_TASK_TO_READY_STATE(pxTCB);

/* Called on each OS tick. Will call uiPortGetTimestamp to make sure it is called at least once every OS tick. */
#undef traceTASK_INCREMENT_TICK

#define traceTASK_INCREMENT_TICK( xTickCount ) \
          { trcKERNEL_HOOKS_INCREMENT_TICK(); }
/* FIXME: It will may be the following.  */
/*
#define traceTASK_INCREMENT_TICK( xTickCount ) \
          if (OSRunning == 1) { trcKERNEL_HOOKS_NEW_TIME(DIV_NEW_TIME, OSTickCtr + 1); } \
          else if (OSRunning == 0) { trcKERNEL_HOOKS_INCREMENT_TICK(); }
*/

/* Called on each task-switch */
#undef traceTASK_SWITCHED_IN
#define traceTASK_SWITCHED_IN(ptcb) \
		trcKERNEL_HOOKS_TASK_SWITCH(TRACE_GET_CURRENT_TASK());

/* Called on vTaskSuspend */
#undef traceTASK_SUSPEND
#define traceTASK_SUSPEND( pxTaskToSuspend ) \
	trcKERNEL_HOOKS_TASK_SUSPEND(TASK_SUSPEND, pxTaskToSuspend);

/* Called from special case with timer only */
#undef traceTASK_DELAY_SUSPEND
#define traceTASK_DELAY_SUSPEND( pxTaskToSuspend ) \
	trcKERNEL_HOOKS_TASK_SUSPEND(TASK_SUSPEND, pxTaskToSuspend); \
	trcKERNEL_HOOKS_SET_TASK_INSTANCE_FINISHED();

/* Called on vTaskDelay - note the use of FreeRTOS variable xTicksToDelay */
#undef traceTASK_DELAY
#define traceTASK_DELAY(xTicksToDelay) \
	trcKERNEL_HOOKS_TASK_DELAY(TASK_DELAY, TRACE_GET_CURRENT_TASK(), xTicksToDelay); \
	trcKERNEL_HOOKS_SET_TASK_INSTANCE_FINISHED();

/* Called on vTaskDelayUntil - note the use of FreeRTOS variable xTimeToWake */
#undef traceTASK_DELAY_UNTIL
#define traceTASK_DELAY_UNTIL() \
	trcKERNEL_HOOKS_TASK_DELAY(TASK_DELAY_UNTIL, pxCurrentTCB, xTimeToWake); \
	trcKERNEL_HOOKS_SET_TASK_INSTANCE_FINISHED();

/* Called on vTaskDelete */
#undef traceTASK_DELETE
#define traceTASK_DELETE( pxTaskToDelete ) \
	{ TRACE_SR_ALLOC_CRITICAL_SECTION(); \
	TRACE_ENTER_CRITICAL_SECTION(); \
	trcKERNEL_HOOKS_TASK_DELETE(DELETE_OBJ, pxTaskToDelete); \
	TRACE_EXIT_CRITICAL_SECTION(); }

/* Called on vQueueDelete */
#undef traceQUEUE_DELETE
#define traceQUEUE_DELETE( pxQueue ) \
	{ TRACE_SR_ALLOC_CRITICAL_SECTION(); \
	TRACE_ENTER_CRITICAL_SECTION(); \
	trcKERNEL_HOOKS_OBJECT_DELETE(DELETE_OBJ, UNUSED, pxQueue); \
	TRACE_EXIT_CRITICAL_SECTION(); }

/* Called on OSSemDel */
/* No need to protect critical section. */
#undef traceSEM_DELETE
#define traceSEM_DELETE(CLASS, p_sem) \
   trcKERNEL_HOOKS_OBJECT_DELETE(DELETE_OBJ, CLASS, p_sem);

/* Called on OSQDel */
/* No need to protect critical section. */
#undef traceQ_DELETE
#define traceQ_DELETE(CLASS, p_q) \
   trcKERNEL_HOOKS_OBJECT_DELETE(DELETE_OBJ, CLASS, p_q);

/* Called on OSMutexDel */
/* No need to protect critical section. */
#undef traceMUTEX_DELETE
#define traceMUTEX_DELETE(CLASS, p_mutex) \
   trcKERNEL_HOOKS_OBJECT_DELETE(DELETE_OBJ, CLASS, p_mutex);

/* Called on vTaskCreate */
#undef traceTASK_CREATE
#define traceTASK_CREATE(pxNewTCB) \
	if (pxNewTCB != NULL) \
	{ \
		trcKERNEL_HOOKS_TASK_CREATE(CREATE_OBJ, UNUSED, pxNewTCB); \
	}

/* Called in vTaskCreate, if it fails (typically if the stack can not be allocated) */
#undef traceTASK_CREATE_FAILED
#define traceTASK_CREATE_FAILED(ptcb) \
	trcKERNEL_HOOKS_TASK_CREATE_FAILED(CREATE_OBJ, UNUSED);

#define traceTASK_MSG_Q_CREATE(CLASS, p_msg_q, p_name )\
   TRACE_ENTER_CRITICAL_SECTION(); \
   trcKERNEL_HOOKS_OBJECT_CREATE(CREATE_OBJ, CLASS, p_msg_q); \
   trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, p_msg_q), TRACE_GET_OBJECT_NUMBER(CLASS, p_msg_q), p_name); \
   TRACE_EXIT_CRITICAL_SECTION();

#undef traceTASK_SEM_CREATE
#define traceTASK_SEM_CREATE(CLASS, p_tcb, p_name )\
   TRACE_ENTER_CRITICAL_SECTION(); \
   trcKERNEL_HOOKS_OBJECT_CREATE(CREATE_OBJ, CLASS, p_tcb); \
   trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, p_tcb), TRACE_GET_OBJECT_NUMBER(CLASS, p_tcb), p_name); \
   TRACE_EXIT_CRITICAL_SECTION();

/* Called in xQueueCreate, and thereby for all other object based on queues, such as semaphores. */
#undef traceQUEUE_CREATE
#define traceQUEUE_CREATE(CLASS, pxNewQueue,p_name )\
	trcKERNEL_HOOKS_OBJECT_CREATE(CREATE_OBJ, UNUSED, pxNewQueue); \
  trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxNewQueue), TRACE_GET_OBJECT_NUMBER(CLASS, pxNewQueue), p_name);
#undef traceQ_CREATE
#define traceQ_CREATE(CLASS, pxNewQueue, p_name)\
	trcKERNEL_HOOKS_OBJECT_CREATE(CREATE_OBJ, CLASS, pxNewQueue); \
  trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pxNewQueue), TRACE_GET_OBJECT_NUMBER(CLASS, pxNewQueue), p_name);

/* Called in xQueueCreate, if the queue creation fails */
#undef traceQUEUE_CREATE_FAILED
#define traceQUEUE_CREATE_FAILED( queueType ) \
	trcKERNEL_HOOKS_OBJECT_CREATE_FAILED(CREATE_OBJ, UNUSED, queueType);
#undef traceQ_CREATE_FAILED
#define traceQ_CREATE_FAILED( queueType ) \
	trcKERNEL_HOOKS_OBJECT_CREATE_FAILED(CREATE_OBJ, UNUSED, queueType);

/* Called in xQueueCreateMutex, and thereby also from xSemaphoreCreateMutex and xSemaphoreCreateRecursiveMutex */
#undef traceMUTEX_CREATE
#define traceMUTEX_CREATE(CLASS, pmutex,p_name ) \
	trcKERNEL_HOOKS_OBJECT_CREATE(CREATE_OBJ, CLASS, pmutex); \
  trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, pmutex), TRACE_GET_OBJECT_NUMBER(CLASS, pmutex), p_name);

/* Called in xQueueCreateMutex when the operation fails (when memory allocation fails) */
#undef traceMUTEX_CREATE_FAILED
#define traceMUTEX_CREATE_FAILED() \
	trcKERNEL_HOOKS_OBJECT_CREATE_FAILED(CREATE_OBJ, UNUSED, queueQUEUE_TYPE_MUTEX);

#undef traceSEM_CREATE
#define traceSEM_CREATE(CLASS, p_sem, p_name )\
   trcKERNEL_HOOKS_OBJECT_CREATE(CREATE_OBJ, CLASS, p_sem); \
   trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, p_sem), TRACE_GET_OBJECT_NUMBER(CLASS, p_sem), p_name);

#undef traceMEM_CREATE
#define traceMEM_CREATE(CLASS, p_mem, p_name )\
   trcKERNEL_HOOKS_OBJECT_CREATE(CREATE_OBJ, CLASS, p_mem); \
   trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(CLASS, p_mem), TRACE_GET_OBJECT_NUMBER(CLASS, p_mem), p_name);

/* Called when a message is sent to a queue */	/* CS IS NEW ! */
#undef traceQUEUE_SEND
#define traceQUEUE_SEND( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, SUCCESS, UNUSED, pxQueue); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_q, (uint8_t)(p_q->PendList.NbrEntries));

/* Called when a message failed to be sent to a queue (timeout) */
#undef traceQUEUE_SEND_FAILED
#define traceQUEUE_SEND_FAILED( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, FAILED, UNUSED, pxQueue);

/* Called when a message is sent to a queue */
#undef traceTASK_MSG_Q_SEND
#define traceTASK_MSG_Q_SEND(CLASS, p_msg_q) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, SUCCESS, CLASS, p_msg_q); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_msg_q, (uint32_t)((OS_MSG_Q *)(p_msg_q)->NbrEntries));

#undef traceTASK_MSG_Q_SEND_FAILED
#define traceTASK_MSG_Q_SEND_FAILED(CLASS, p_msg_q) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, FAILED, CLASS, p_msg_q); 


/* Called when a signal is posted to a task semaphore (Post) */
#undef traceTASK_SEM_SEND
#define traceTASK_SEM_SEND(CLASS, p_tcb) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, SUCCESS, CLASS, p_tcb); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_tcb, (uint8_t)(p_tcb->SemCtr));

#undef traceTASK_SEM_SEND_FAILED
#define traceTASK_SEM_SEND_FAILED(CLASS, p_tcb) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, FAILED, CLASS, p_tcb);

/* Called when a signal is posted to a semaphore (Post) */
#undef traceSEM_SEND
#define traceSEM_SEND(CLASS, p_sem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, SUCCESS, CLASS, p_sem); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_sem, (uint8_t)(p_sem->Ctr));

#undef traceSEM_SEND_FAILED
#define traceSEM_SEND_FAILED(CLASS, p_sem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, FAILED, CLASS, p_sem);

/* Called when a signal is posted to a mutex (Post) */
#undef traceMUTEX_SEND
#define traceMUTEX_SEND(CLASS, p_mutex) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, SUCCESS, CLASS, p_mutex);

#undef traceMUTEX_SEND_FAILED
#define traceMUTEX_SEND_FAILED(CLASS, p_mutex) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, FAILED, CLASS, p_mutex);

/* Called when a task or ISR returns a memory partition (put) */
#undef traceMEM_SEND
#define traceMEM_SEND(CLASS, p_mem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, SUCCESS, CLASS, p_mem); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_mem, (uint8_t)(p_mem->NbrFree));

#undef traceMEM_SEND_FAILED
#define traceMEM_SEND_FAILED(CLASS, p_mem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, FAILED, CLASS, p_mem);


/* Called when a message is received from a task message queue */
#undef traceTASK_MSG_Q_RECEIVE
#define traceTASK_MSG_Q_RECEIVE(CLASS, p_msg_q) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, SUCCESS, CLASS, p_msg_q); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_msg_q, (uint8_t)(p_msg_q->NbrEntries));

#undef traceTASK_MSG_Q_RECEIVE_FAILED
#define traceTASK_MSG_Q_RECEIVE_FAILED(CLASS, p_msg_q) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, FAILED, CLASS, p_msg_q);

/* Called when the task is blocked due to a send operation on a full queue */
#undef traceBLOCKING_ON_QUEUE_SEND
#define traceBLOCKING_ON_QUEUE_SEND( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, BLOCK, UNUSED, pxQueue);

/* Called when a message is received from a task message queue */
#undef traceTASK_MSG_Q_RECEIVE_BLOCK
#define traceTASK_MSG_Q_RECEIVE_BLOCK(CLASS, p_msg_q) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, BLOCK, CLASS, p_msg_q); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_msg_q, (uint8_t)(p_msg_q->NbrEntries));

/* Called when a message is received from a queue */
#undef traceQUEUE_RECEIVE
#define traceQUEUE_RECEIVE( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, SUCCESS, UNUSED, pxQueue); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_q, (uint8_t)(p_q->PendList.NbrEntries));
#undef traceQ_RECEIVE
#define traceQ_RECEIVE(CLASS, pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, SUCCESS, CLASS, pxQueue); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_q, (uint8_t)(p_q->PendList.NbrEntries));
/* Called when a message is received from a message queue */
#undef traceQ_RECEIVE_BLOCK
#define traceQ_RECEIVE_BLOCK(CLASS, p_q) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, BLOCK, CLASS, p_q); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_q, (uint8_t)(p_q->PendList.NbrEntries));

#undef traceQ_SEND
#define traceQ_SEND(CLASS, p_q) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, SUCCESS, CLASS, p_q); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_q, (uint8_t)(p_q->PendList.NbrEntries));

#undef traceQ_SEND_FAILED
#define traceQ_SEND_FAILED(CLASS, p_q) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND, FAILED, CLASS, p_q);


/* Called when a receive operation on a queue fails (timeout) */
#undef traceQUEUE_RECEIVE_FAILED
#define traceQUEUE_RECEIVE_FAILED( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, FAILED, UNUSED, pxQueue);
#undef traceQ_RECEIVE_FAILED
#define traceQ_RECEIVE_FAILED(CLASS, pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, FAILED, CLASS, pxQueue);

/* Called when the task is blocked due to a receive operation on an empty queue */
#undef traceBLOCKING_ON_QUEUE_RECEIVE
#define traceBLOCKING_ON_QUEUE_RECEIVE( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, BLOCK, UNUSED, pxQueue); \
	if (TRACE_GET_OBJECT_TRACE_CLASS(UNUSED, pxQueue) != TRACE_CLASS_MUTEX) \
	{trcKERNEL_HOOKS_SET_TASK_INSTANCE_FINISHED();}
#undef traceBLOCKING_ON_Q_RECEIVE
#define traceBLOCKING_ON_Q_RECEIVE( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, BLOCK, UNUSED, pxQueue); \
	if (TRACE_GET_OBJECT_TRACE_CLASS(UNUSED, pxQueue) != TRACE_CLASS_MUTEX) \
	{trcKERNEL_HOOKS_SET_TASK_INSTANCE_FINISHED();}

/* Called on xQueuePeek */
#undef traceQUEUE_PEEK
#define traceQUEUE_PEEK( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(PEEK, SUCCESS, UNUSED, pxQueue);
#undef traceQ_PEEK
#define traceQ_PEEK( pxQueue ) \
	trcKERNEL_HOOKS_KERNEL_SERVICE(PEEK, SUCCESS, UNUSED, pxQueue);

/* Called when a task pends on a task semaphore (Pend)*/
#undef traceTASK_SEM_RECEIVE
#define traceTASK_SEM_RECEIVE(CLASS, p_tcb) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, SUCCESS, CLASS, p_tcb); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_tcb, (uint8_t)(p_tcb->SemCtr));

#undef traceTASK_SEM_RECEIVE_FAILED
#define traceTASK_SEM_RECEIVE_FAILED(CLASS, p_tcb) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, FAILED, CLASS, p_tcb);

/* Called when a task pends and blocks on a task semaphore (Pend)*/
#undef traceTASK_SEM_RECEIVE_BLOCK
#define traceTASK_SEM_RECEIVE_BLOCK(CLASS, p_tcb) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, BLOCK, CLASS, p_tcb); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_tcb, (uint8_t)(p_tcb->SemCtr));

/* Called when a task pends on a semaphore (Pend)*/
#undef traceSEM_RECEIVE
#define traceSEM_RECEIVE(CLASS, p_sem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, SUCCESS, CLASS, p_sem); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_sem, (uint8_t)(p_sem->Ctr));

#undef traceSEM_RECEIVE_FAILED
#define traceSEM_RECEIVE_FAILED(CLASS, p_sem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, FAILED, CLASS, p_sem);

/* Called when a task pends on a semaphore (Pend)*/
#undef traceSEM_RECEIVE_BLOCK
#define traceSEM_RECEIVE_BLOCK(CLASS, p_sem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, BLOCK, CLASS, p_sem); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_sem, (uint8_t)(p_sem->Ctr));

#undef traceMUTEX_RECEIVE
#define traceMUTEX_RECEIVE(CLASS, p_mutex) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, SUCCESS, CLASS, p_mutex);

#undef traceMUTEX_RECEIVE_FAILED
#define traceMUTEX_RECEIVE_FAILED(CLASS, p_mutex) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, FAILED, CLASS, p_mutex);

/* Called when a task pends on a mutex (Pend)*/
#undef traceMUTEX_RECEIVE_BLOCK
#define traceMUTEX_RECEIVE_BLOCK(CLASS, p_mutex) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, BLOCK, CLASS, p_mutex);

/* Called when a task or ISR gets a memory partition */
#undef traceMEM_RECEIVE
#define traceMEM_RECEIVE(CLASS, p_mem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, SUCCESS, CLASS, p_mem); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(CLASS, p_mem, (uint8_t)(p_mem->NbrFree));

#undef traceMEM_RECEIVE_FAILED
#define traceMEM_RECEIVE_FAILED(CLASS, p_mem) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE, FAILED, CLASS, p_mem);

/* Called when a message is sent from interrupt context, e.g., using
   xQueueSendFromISR */
#undef traceQUEUE_SEND_TO_TASK
#define traceQUEUE_SEND_TO_TASK( pxQueue ) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND_TO_TASK, SUCCESS, UNUSED, pxQueue); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(UNUSED, pxQueue, (uint8_t)(pxQueue->uxMessagesWaiting + 1));
#undef traceQ_SEND_TO_TASK
#define traceQ_SEND_TO_TASK( pxQueue ) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND_TO_TASK, SUCCESS, UNUSED, pxQueue); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(UNUSED, pxQueue, (uint8_t)(pxQueue->uxMessagesWaiting + 1));

/* Called when a message send from interrupt context fails (since the queue
   was full) */
#undef traceQUEUE_SEND_TO_TASK_FAILED
#define traceQUEUE_SEND_TO_TASK_FAILED( pxQueue ) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND_TO_TASK, FAILED, UNUSED, pxQueue);
#undef traceQ_SEND_TO_TASK_FAILED
#define traceQ_SEND_TO_TASK_FAILED( pxQueue ) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(SEND_TO_TASK, FAILED, UNUSED, pxQueue);

/* Called when a message is received in interrupt context, e.g., using
   xQueueReceiveFromISR */
#undef traceQUEUE_RECEIVE_FROM_TASK
#define traceQUEUE_RECEIVE_FROM_TASK( pxQueue ) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE_FROM_TASK, SUCCESS, UNUSED, pxQueue); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(UNUSED, pxQueue, (uint8_t)(pxQueue->uxMessagesWaiting - 1));
#undef traceQ_RECEIVE_FROM_TASK
#define traceQ_RECEIVE_FROM_TASK( pxQueue ) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE_FROM_TASK, SUCCESS, UNUSED, pxQueue); \
  trcKERNEL_HOOKS_SET_OBJECT_STATE(UNUSED, pxQueue, (uint8_t)(pxQueue->uxMessagesWaiting - 1));

/* Called when a message receive from interrupt context fails (since the queue
   was empty) */
#undef traceQUEUE_RECEIVE_FROM_TASK_FAILED
#define traceQUEUE_RECEIVE_FROM_TASK_FAILED( pxQueue ) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE_FROM_TASK, FAILED, UNUSED, pxQueue);
#undef traceQ_RECEIVE_FROM_TASK_FAILED
#define traceQ_RECEIVE_FROM_TASK_FAILED( pxQueue ) \
   trcKERNEL_HOOKS_KERNEL_SERVICE(RECEIVE_FROM_TASK, FAILED, UNUSED, pxQueue);

/* Called in vTaskPrioritySet */
#undef traceTASK_PRIORITY_SET
#define traceTASK_PRIORITY_SET( pxTask, uxNewPriority ) \
	trcKERNEL_HOOKS_TASK_PRIORITY_CHANGE(TASK_PRIORITY_SET, pxTask, uxNewPriority);

/* Called in vTaskPriorityInherit, which is called by Mutex operations */
#undef traceTASK_PRIORITY_INHERIT
#define traceTASK_PRIORITY_INHERIT( pxTask, uxNewPriority ) \
	trcKERNEL_HOOKS_TASK_PRIORITY_CHANGE(TASK_PRIORITY_INHERIT, pxTask, uxNewPriority);

/* Called in vTaskPriorityDisinherit, which is called by Mutex operations */
#undef traceTASK_PRIORITY_DISINHERIT
#define traceTASK_PRIORITY_DISINHERIT( pxTask, uxNewPriority ) \
	trcKERNEL_HOOKS_TASK_PRIORITY_CHANGE(TASK_PRIORITY_DISINHERIT, pxTask, uxNewPriority);

/* Called in vTaskResume */
#undef traceTASK_RESUME
#define traceTASK_RESUME( pxTaskToResume ) \
	trcKERNEL_HOOKS_TASK_RESUME(TASK_RESUME, pxTaskToResume);

/* Called in vTaskResumeFromISR */
#undef traceTASK_RESUME_FROM_ISR
#define traceTASK_RESUME_FROM_ISR( pxTaskToResume ) \
	trcKERNEL_HOOKS_TASK_RESUME(TASK_RESUME_FROM_ISR, pxTaskToResume);

#undef traceISR_REGISTER
#define traceISR_REGISTER(isr_id, isr_name, isr_prio) \
         vTraceSetISRProperties(isr_id, isr_name, isr_prio);

#undef traceISR_BEGIN
#define traceISR_BEGIN(isr_id) \
         vTraceStoreISRBegin(isr_id);

#undef traceISR_END
#define traceISR_END() \
         vTraceStoreISREnd();

#if (INCLUDE_MEMMANG_EVENTS == 1)

extern void trace_store_mem_mang_event(uint32_t ecode, uint32_t address, int32_t size);

#undef traceMALLOC
#define traceMALLOC( pvAddress, uiSize ) {if (pvAddress != 0) trace_store_mem_mang_event(MEM_MALLOC_SIZE, ( uint32_t ) pvAddress, (int32_t)uiSize); }

#undef traceFREE
#define traceFREE( pvAddress, uiSize ) {trace_store_mem_mang_event(MEM_FREE_SIZE, ( uint32_t ) pvAddress, (int32_t)(-uiSize)); }

#endif

/************************************************************************/
/* KERNEL SPECIFIC MACROS TO EXCLUDE OR INCLUDE THINGS IN TRACE			*/
/************************************************************************/

/* Returns the exclude state of the object */
uint8_t trace_is_object_excluded(traceObjectClass objectclass, object_handle_t handle);

#define TRACE_SET_QUEUE_FLAG_ISEXCLUDED(queueIndex) TRACE_SET_FLAG_ISEXCLUDED(excluded_objects, queueIndex)
#define TRACE_CLEAR_QUEUE_FLAG_ISEXCLUDED(queueIndex) TRACE_CLEAR_FLAG_ISEXCLUDED(excluded_objects, queueIndex)
#define TRACE_GET_QUEUE_FLAG_ISEXCLUDED(queueIndex) TRACE_GET_FLAG_ISEXCLUDED(excluded_objects, queueIndex)

#define TRACE_SET_SEMAPHORE_FLAG_ISEXCLUDED(semaphoreIndex) TRACE_SET_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+semaphoreIndex)
#define TRACE_CLEAR_SEMAPHORE_FLAG_ISEXCLUDED(semaphoreIndex) TRACE_CLEAR_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+semaphoreIndex)
#define TRACE_GET_SEMAPHORE_FLAG_ISEXCLUDED(semaphoreIndex) TRACE_GET_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+semaphoreIndex)

#define TRACE_SET_MUTEX_FLAG_ISEXCLUDED(mutexIndex) TRACE_SET_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+NSemaphore+1+mutexIndex)
#define TRACE_CLEAR_MUTEX_FLAG_ISEXCLUDED(mutexIndex) TRACE_CLEAR_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+NSemaphore+1+mutexIndex)
#define TRACE_GET_MUTEX_FLAG_ISEXCLUDED(mutexIndex) TRACE_GET_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+NSemaphore+1+mutexIndex)

#define TRACE_SET_MEM_FLAG_ISEXCLUDED(memIndex)   TRACE_SET_FLAG_ISEXCLUDED(excluded_objects,   NQueue+1+NSemaphore+1+NMutex+1+memIndex)
#define TRACE_CLEAR_MEM_FLAG_ISEXCLUDED(memIndex) TRACE_CLEAR_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+NSemaphore+1+NMutex+1+memIndex)
#define TRACE_GET_MEM_FLAG_ISEXCLUDED(memIndex)   TRACE_GET_FLAG_ISEXCLUDED(excluded_objects,   NQueue+1+NSemaphore+1+NMutex+1+memIndex)

#define TRACE_SET_TASK_SEM_FLAG_ISEXCLUDED(taskSemIndex)   TRACE_SET_FLAG_ISEXCLUDED(excluded_objects,   NQueue+1+NSemaphore+1+NMutex+1+NMem+1+taskSemIndex)
#define TRACE_CLEAR_TASK_SEM_FLAG_ISEXCLUDED(taskSemIndex) TRACE_CLEAR_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+NSemaphore+1+NMutex+1+NMem+1+taskSemIndex)
#define TRACE_GET_TASK_SEM_FLAG_ISEXCLUDED(taskSemIndex)   TRACE_GET_FLAG_ISEXCLUDED(excluded_objects,   NQueue+1+NSemaphore+1+NMutex+1+NMem+1+taskSemIndex)

#define TRACE_SET_TASK_Q_FLAG_ISEXCLUDED(taskQIndex)   TRACE_SET_FLAG_ISEXCLUDED(excluded_objects,   NQueue+1+NSemaphore+1+NMutex+1+NMem+1+NTaskSem+1+taskQIndex)
#define TRACE_CLEAR_TASK_Q_FLAG_ISEXCLUDED(taskQIndex) TRACE_CLEAR_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+NSemaphore+1+NMutex+1+NMem+1+NTaskSem+1+taskQIndex)
#define TRACE_GET_TASK_Q_FLAG_ISEXCLUDED(taskQIndex)   TRACE_GET_FLAG_ISEXCLUDED(excluded_objects,   NQueue+1+NSemaphore+1+NMutex+1+NMem+1+NTaskSem+1+taskQIndex)

#define TRACE_SET_TASK_FLAG_ISEXCLUDED(taskIndex)   TRACE_SET_FLAG_ISEXCLUDED(excluded_objects,   NQueue+1+NSemaphore+1+NMutex+1+NMem+1+NTaskSem+1+NTaskQ+1+taskIndex)
#define TRACE_CLEAR_TASK_FLAG_ISEXCLUDED(taskIndex) TRACE_CLEAR_FLAG_ISEXCLUDED(excluded_objects, NQueue+1+NSemaphore+1+NMutex+1+NMem+1+NTaskSem+1+NTaskQ+1+taskIndex)
#define TRACE_GET_TASK_FLAG_ISEXCLUDED(taskIndex)   TRACE_GET_FLAG_ISEXCLUDED(excluded_objects,   NQueue+1+NSemaphore+1+NMutex+1+NMem+1+NTaskSem+1+NTaskQ+1+taskIndex)

#define TRACE_CLEAR_OBJECT_FLAG_ISEXCLUDED(objectclass, handle) \
 switch (objectclass) \
{ \
case TRACE_CLASS_QUEUE: \
    TRACE_CLEAR_QUEUE_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_SEMAPHORE: \
    TRACE_CLEAR_SEMAPHORE_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_MUTEX: \
    TRACE_CLEAR_MUTEX_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_MEM: \
    TRACE_CLEAR_MEM_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_TASK_SEM: \
    TRACE_CLEAR_TASK_SEM_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_TASK_Q: \
    TRACE_CLEAR_TASK_Q_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_TASK: \
    TRACE_CLEAR_TASK_FLAG_ISEXCLUDED(handle); \
    break; \
}

#define TRACE_SET_OBJECT_FLAG_ISEXCLUDED(objectclass, handle) \
 switch (objectclass) \
{ \
case TRACE_CLASS_QUEUE: \
    TRACE_SET_QUEUE_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_SEMAPHORE: \
    TRACE_SET_SEMAPHORE_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_MUTEX: \
    TRACE_SET_MUTEX_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_MEM: \
    TRACE_SET_MEM_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_TASK_SEM: \
    TRACE_SET_TASK_SEM_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_TASK_Q: \
    TRACE_SET_TASK_Q_FLAG_ISEXCLUDED(handle); \
    break; \
case TRACE_CLASS_TASK: \
    TRACE_SET_TASK_FLAG_ISEXCLUDED(handle); \
    break; \
}

/* Task */
#define vTraceIncludeTaskInTrace(handle) \
TRACE_CLEAR_TASK_FLAG_ISEXCLUDED(TRACE_GET_TASK_NUMBER(handle));


/* Queue */
#define vTraceExcludeQueueFromTrace(handle) \
TRACE_SET_QUEUE_FLAG_ISEXCLUDED(TRACE_GET_OBJECT_NUMBER(UNUSED, handle));

#define vTraceIncludeQueueInTrace(handle) \
TRACE_CLEAR_QUEUE_FLAG_ISEXCLUDED(TRACE_GET_OBJECT_NUMBER(UNUSED, handle));


/* Semaphore */
#define vTraceExcludeSemaphoreFromTrace(handle) \
TRACE_SET_SEMAPHORE_FLAG_ISEXCLUDED(TRACE_GET_OBJECT_NUMBER(UNUSED, handle));

#define vTraceIncludeSemaphoreInTrace(handle) \
TRACE_CLEAR_QUEUE_FLAG_ISEXCLUDED(TRACE_GET_OBJECT_NUMBER(UNUSED, handle));


/* Mutex */
#define vTraceExcludeMutexFromTrace(handle) \
TRACE_SET_MUTEX_FLAG_ISEXCLUDED(TRACE_GET_OBJECT_NUMBER(UNUSED, handle));

#define vTraceIncludeMutexInTrace(handle) \
TRACE_CLEAR_QUEUE_FLAG_ISEXCLUDED(TRACE_GET_OBJECT_NUMBER(UNUSED, handle));




/* Kernel Services */
#define vTraceExcludeKernelServiceDelayFromTrace() \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(TASK_DELAY); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(TASK_DELAY_UNTIL);

#define vTraceIncludeKernelServiceDelayInTrace() \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(TASK_DELAY); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(TASK_DELAY_UNTIL);

/* HELPER MACROS FOR KERNEL SERVICES FOR OBJECTS */
#define vTraceExcludeKernelServiceSendFromTrace_HELPER(class) \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_SUCCESS + class); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_BLOCK + class); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_FAILED + class); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_FROM_ISR_SUCCESS + class); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_FROM_ISR_FAILED + class);

#define vTraceIncludeKernelServiceSendInTrace_HELPER(class) \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_SUCCESS + class); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_BLOCK + class); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_FAILED + class); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_FROM_ISR_SUCCESS + class); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_SEND_FROM_ISR_FAILED + class);

#define vTraceExcludeKernelServiceReceiveFromTrace_HELPER(class) \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_SUCCESS + class); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_BLOCK + class); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_FAILED + class); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_FROM_ISR_SUCCESS + class); \
TRACE_SET_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_FROM_ISR_FAILED + class);

#define vTraceIncludeKernelServiceReceiveInTrace_HELPER(class) \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_SUCCESS + class); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_BLOCK + class); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_FAILED + class); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_FROM_ISR_SUCCESS + class); \
TRACE_CLEAR_EVENT_CODE_FLAG_ISEXCLUDED(EVENTGROUP_RECEIVE_FROM_ISR_FAILED + class);

/* EXCLUDE AND INCLUDE FOR QUEUE */
#define vTraceExcludeKernelServiceQueueSendFromTrace() \
vTraceExcludeKernelServiceSendFromTrace_HELPER(TRACE_CLASS_QUEUE);

#define vTraceIncludeKernelServiceQueueSendInTrace() \
vTraceIncludeKernelServiceSendInTrace_HELPER(TRACE_CLASS_QUEUE);

#define vTraceExcludeKernelServiceQueueReceiveFromTrace() \
vTraceExcludeKernelServiceReceiveFromTrace_HELPER(TRACE_CLASS_QUEUE);

#define vTraceIncludeKernelServiceQueueReceiveInTrace() \
vTraceIncludeKernelServiceReceiveInTrace_HELPER(TRACE_CLASS_QUEUE);

/* EXCLUDE AND INCLUDE FOR SEMAPHORE */
#define vTraceExcludeKernelServiceSemaphoreSendFromTrace() \
vTraceExcludeKernelServiceSendFromTrace_HELPER(TRACE_CLASS_SEMAPHORE);

#define vTraceIncludeKernelServicSemaphoreSendInTrace() \
vTraceIncludeKernelServiceSendInTrace_HELPER(TRACE_CLASS_SEMAPHORE);

#define vTraceExcludeKernelServiceSemaphoreReceiveFromTrace() \
vTraceExcludeKernelServiceReceiveFromTrace_HELPER(TRACE_CLASS_SEMAPHORE);

#define vTraceIncludeKernelServiceSemaphoreReceiveInTrace() \
vTraceIncludeKernelServiceReceiveInTrace_HELPER(TRACE_CLASS_SEMAPHORE);

/* EXCLUDE AND INCLUDE FOR MUTEX */
#define vTraceExcludeKernelServiceMutexSendFromTrace() \
vTraceExcludeKernelServiceSendFromTrace_HELPER(TRACE_CLASS_MUTEX);

#define vTraceIncludeKernelServiceMutexSendInTrace() \
vTraceIncludeKernelServiceSendInTrace_HELPER(TRACE_CLASS_MUTEX);

#define vTraceExcludeKernelServiceMutexReceiveFromTrace() \
vTraceExcludeKernelServiceReceiveFromTrace_HELPER(TRACE_CLASS_MUTEX);

#define vTraceIncludeKernelServiceMutexReceiveInTrace() \
vTraceIncludeKernelServiceReceiveInTrace_HELPER(TRACE_CLASS_MUTEX);

/************************************************************************/
/* KERNEL SPECIFIC MACROS TO NAME OBJECTS, IF NECESSARY				 */
/************************************************************************/
#define TRACE_SET_QUEUE_NAME(object, name) \
trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(UNUSED, object), TRACE_GET_OBJECT_NUMBER(UNUSED, object), name);

#define TRACE_SET_SEMAPHORE_NAME(object, name) \
trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(UNUSED, object), TRACE_GET_OBJECT_NUMBER(UNUSED, object), name);

#define TRACE_SET_MUTEX_NAME(object, name) \
trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(UNUSED, object), TRACE_GET_OBJECT_NUMBER(UNUSED, object), name);

#define TRACE_SET_EVENTGROUP_NAME(object, name) \
trace_set_object_name(TRACE_CLASS_EVENTGROUP, (object_handle_t)uxEventGroupGetNumber(object), name);

#undef traceQUEUE_REGISTRY_ADD
#define traceQUEUE_REGISTRY_ADD(object, name) trace_set_object_name(TRACE_GET_OBJECT_TRACE_CLASS(UNUSED, object), TRACE_GET_OBJECT_NUMBER(UNUSED, object), name);
#endif

#endif /* TRCKERNELPORTFREERTOS_H_ */
