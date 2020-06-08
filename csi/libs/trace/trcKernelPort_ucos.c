#include "trcKernelPort.h"

#if (USE_TRACEALYZER_RECORDER == 1)

#include <stdint.h>

#include "os.h"

object_handle_t trace_get_task_number(void* handle)
{
  OS_TCB *ptcb;
  if (NULL != handle)
    {
      ptcb = handle;
      return ptcb->TaskID;
    }

  return 0;
}

unsigned char trace_is_scheduler_active()
{
	return OSRunning == OS_STATE_OS_RUNNING;
}

unsigned char trace_is_scheduler_suspended()
{
  return OSRunning == OS_STATE_OS_STOPPED;
}

unsigned char trace_is_scheduler_started()
{
  return OSRunning == OS_STATE_OS_RUNNING;
}

void* trace_get_current_task_handle()
{
  return OSTCBCurPtr;
}

void* trace_get_next_task_handle()
{
  return OSTCBHighRdyPtr;
}


/* Initialization of the object property table */
void trace_init_object_property_table()
{
	g_recorder_data_ptr->object_property_table.number_of_object_classes = TRACE_NCLASSES;
	g_recorder_data_ptr->object_property_table.name_length_per_class    = OBJECT_NAME_LENGTH;
	g_recorder_data_ptr->object_property_table.number_of_objects_per_class[0] = NQueue;
	g_recorder_data_ptr->object_property_table.number_of_objects_per_class[1] = NSemaphore;
	g_recorder_data_ptr->object_property_table.number_of_objects_per_class[2] = NMutex;
	g_recorder_data_ptr->object_property_table.number_of_objects_per_class[3] = NTask;
	g_recorder_data_ptr->object_property_table.number_of_objects_per_class[4] = NISR;
	g_recorder_data_ptr->object_property_table.number_of_objects_per_class[5] = NMem;
	g_recorder_data_ptr->object_property_table.number_of_objects_per_class[6] = NTaskSem;	
	g_recorder_data_ptr->object_property_table.number_of_objects_per_class[7] = NTaskQ;	
	g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[0] = PropertyTableSizeQueue;
	g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[1] = PropertyTableSizeSemaphore;
	g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[2] = PropertyTableSizeMutex;
	g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[3] = PropertyTableSizeTask;
	g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[4] = PropertyTableSizeISR;
	g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[5] = PropertyTableSizeMem;
	g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[6] = PropertyTableSizeTaskSem;
	g_recorder_data_ptr->object_property_table.total_property_bytes_per_class[7] = PropertyTableSizeTaskQ;
	g_recorder_data_ptr->object_property_table.start_index_of_class[0] = StartIndexQueue;
	g_recorder_data_ptr->object_property_table.start_index_of_class[1] = StartIndexSemaphore;
	g_recorder_data_ptr->object_property_table.start_index_of_class[2] = StartIndexMutex;
	g_recorder_data_ptr->object_property_table.start_index_of_class[3] = StartIndexTask;
	g_recorder_data_ptr->object_property_table.start_index_of_class[4] = StartIndexISR;
	g_recorder_data_ptr->object_property_table.start_index_of_class[5] = StartIndexMem;
	g_recorder_data_ptr->object_property_table.start_index_of_class[6] = StartIndexTaskSem;
	g_recorder_data_ptr->object_property_table.start_index_of_class[7] = StartIndexTaskQ;
	g_recorder_data_ptr->object_property_table.object_property_table_size = TRACE_OBJECT_TABLE_SIZE;
}

/* Initialization of the handle mechanism, see e.g, trace_get_object_handle */
void trace_init_object_handle_stack()
{
	object_handle_table.available_index[TRACE_CLASS_QUEUE] = 0;
	object_handle_table.available_index[TRACE_CLASS_SEMAPHORE] = NQueue;
	object_handle_table.available_index[TRACE_CLASS_TASK] = NQueue + NSemaphore + NMutex;
	object_handle_table.available_index[TRACE_CLASS_MUTEX] = NQueue + NSemaphore;
	object_handle_table.available_index[TRACE_CLASS_ISR] = NQueue + NSemaphore + NMutex + NTask;
	object_handle_table.available_index[TRACE_CLASS_MEM] = NQueue + NSemaphore + NMutex + NTask + NISR;
	object_handle_table.available_index[TRACE_CLASS_TASK_SEM] = NQueue + NSemaphore + NMutex + NTask + NISR + NMem;
	object_handle_table.available_index[TRACE_CLASS_TASK_Q] = NQueue + NSemaphore + NMutex + NTask + NISR + NMem + NTaskSem;
	object_handle_table.available_index[TRACE_NCLASSES] = TRACE_KERNEL_OBJECT_COUNT;

  object_handle_table.start_index[TRACE_CLASS_QUEUE] = 0;
  object_handle_table.start_index[TRACE_CLASS_SEMAPHORE] = NQueue;
  object_handle_table.start_index[TRACE_CLASS_MUTEX] = NQueue + NSemaphore;
  object_handle_table.start_index[TRACE_CLASS_TASK] = NQueue + NSemaphore + NMutex;
  object_handle_table.start_index[TRACE_CLASS_ISR] = NQueue + NSemaphore + NMutex + NTask;
  object_handle_table.start_index[TRACE_CLASS_MEM] = NQueue + NSemaphore + NMutex + NTask + NISR;
  object_handle_table.start_index[TRACE_CLASS_TASK_SEM] = NQueue + NSemaphore + NMutex + NTask + NISR + NMem;
  object_handle_table.start_index[TRACE_CLASS_TASK_Q] = NQueue + NSemaphore + NMutex + NTask + NISR + NMem + NTaskSem;
  object_handle_table.start_index[TRACE_NCLASSES] = TRACE_KERNEL_OBJECT_COUNT;
}
	
/* Returns the "Not enough handles" error message for this object class */
const char* trace_get_error_not_enough_handles(traceObjectClass objectclass)
{
	switch(objectclass)
	{
	case TRACE_CLASS_TASK:
		return "Not enough TASK handles - increase NTask in trcConfig.h";
	case TRACE_CLASS_ISR:
		return "Not enough ISR handles - increase NISR in trcConfig.h";
	case TRACE_CLASS_SEMAPHORE:
		return "Not enough SEMAPHORE handles - increase NSemaphore in trcConfig.h";
	case TRACE_CLASS_MUTEX:
		return "Not enough MUTEX handles - increase NMutex in trcConfig.h";
	case TRACE_CLASS_QUEUE:
		return "Not enough QUEUE handles - increase NQueue in trcConfig.h";
  case TRACE_CLASS_MEM:
    return "Not enough MEMORY PARTITION handles - increase NMem in trcConfig.h";
  case TRACE_CLASS_TASK_SEM:
    return "Not enough TASK handles - increase NTask in trcConfig.h";
  case TRACE_CLASS_TASK_Q:
    return "Not enough TASK handles - increase NTask in trcConfig.h";

	default:
		return "pszTraceGetErrorHandles: Invalid objectclass!";
	}
}

/* Returns the exclude state of the object */
uint8_t trace_is_object_excluded(traceObjectClass objectclass, object_handle_t handle)
{
	TRACE_ASSERT(objectclass < TRACE_NCLASSES, "prvTraceIsObjectExcluded: objectclass >= TRACE_NCLASSES", 1);
	TRACE_ASSERT(handle <= g_recorder_data_ptr->object_property_table.number_of_objects_per_class[objectclass], "trace_is_object_excluded: Invalid value for handle", 1);
	
	switch(objectclass)
	{
	case TRACE_CLASS_TASK:
		return TRACE_GET_TASK_FLAG_ISEXCLUDED(handle);
	case TRACE_CLASS_SEMAPHORE:
		return TRACE_GET_SEMAPHORE_FLAG_ISEXCLUDED(handle);
	case TRACE_CLASS_MUTEX:
		return TRACE_GET_MUTEX_FLAG_ISEXCLUDED(handle);
	case TRACE_CLASS_QUEUE:
		return TRACE_GET_QUEUE_FLAG_ISEXCLUDED(handle);
  case TRACE_CLASS_MEM:
    return TRACE_GET_MEM_FLAG_ISEXCLUDED(handle);
  case TRACE_CLASS_TASK_SEM:
    return TRACE_GET_TASK_SEM_FLAG_ISEXCLUDED(handle);
  case TRACE_CLASS_TASK_Q:
    return TRACE_GET_TASK_Q_FLAG_ISEXCLUDED(handle);
	}
	
	trace_error("Invalid object class ID in trace_is_object_excluded!");
	
	/* Must never reach */
	return 1;
}

#endif
